#include "Base/OpenCL/ComputeKernel.h"
#include "AdvancedErosionFilter.h"

#include "Utils.h"

#include <imgui.h>
#include <iostream>
#include <regex>


#define MAX(x, y) x>y?x:y
#define MIN(x, y) x<y?x:y

AdvancedErosionFilter::AdvancedErosionFilter(Model* model)
    : Filter(model, "Wind Erosion Filter (GPU)")
{
}

void AdvancedErosionFilter::Render()
{

    ImGui::DragInt("Local Work Group Size##windErosionFilter", &localWorkSize, 1, 1);


}

nlohmann::json AdvancedErosionFilter::Save()
{
	return nlohmann::json();
}

void AdvancedErosionFilter::Load(nlohmann::json data)
{
}

void AdvancedErosionFilter::Apply()
{
	srand(time(NULL));
	model->mesh->RecalculateNormals();
    bool tmp = false;

    try {
                 
        tmp = false;

        if(model->mesh->vertexCount % localWorkSize != 0)
        {
            std::cout << std::endl << "Invalid Local Work Group Size. Falling back to 1." << std::endl;
            localWorkSize = 1;
        }

        std::string kernelSrc = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\kernels\\advanced_erosion.cl", &tmp);

        // Some constants 
        kernelSrc = std::regex_replace(kernelSrc, std::regex("LOCAL_WORK_SIZE"), std::to_string(localWorkSize));

        kernels.Clear();
        kernels.AddSoruce(kernelSrc);
        kernels.BuildProgram("");
        kernels.AddKernel("erode");

        kernels.CreateBuffer("mesh", CL_MEM_READ_WRITE, sizeof(Vert) * model->mesh->vertexCount);
        kernels.WriteBuffer("mesh", true, sizeof(Vert) * model->mesh->vertexCount, model->mesh->vert);

        kernels.SetKernelArg("erode", 0, "mesh");

        std::cout << std::endl << "Starting processing erosion." << std::endl;

        cl::NDRange global(model->mesh->vertexCount / localWorkSize);
        cl::NDRange local(localWorkSize);
        kernels.ExecuteKernel("erode", local, global);

        std::cout << std::endl << "Finished processing erosion." << std::endl;

        kernels.ReadBuffer("mesh", true, sizeof(Vert) * model->mesh->vertexCount, model->mesh->vert);
    }
    catch (std::exception ex)
    {
        std::cout << "OpenCL Exception : " << ex.what() << "\n";
        return;
    }

    // Deletion 

	model->mesh->RecalculateNormals();
	model->UploadToGPU();
}
 