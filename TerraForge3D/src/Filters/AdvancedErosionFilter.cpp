#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include "CL/opencl.hpp"

#include "AdvancedErosionFilter.h"

#include "Utils.h"

#include <imgui.h>
#include <iostream>
#include <regex>


#define MAX(x, y) x>y?x:y
#define MIN(x, y) x<y?x:y

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
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        if (platforms.size() == 0)
        {
            std::cout << "No Platform found supporting OpenCL!\n";
            return;
        }

        cl::Platform plat = platforms[0];
        std::cout << "Using OpenCL platform : " << plat.getInfo<CL_PLATFORM_NAME>() << "\n";

        std::vector<cl::Device> devices;
        plat.getDevices(CL_DEVICE_TYPE_ALL, &devices);

        if (devices.size() == 0)
        {
            std::cout << "No Device found supporting OpenCL!\n";
            return;
        }

        cl::Device dev;

        tmp = false;
        for (auto& d : devices)
        {
            tmp = (d.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU);
            if (tmp)
            {
                dev = d;
                std::cout << "Using GPU Device : " << dev.getInfo<CL_DEVICE_NAME>() << "\n";
                break;
            }
        }
        if (!tmp) {
            dev = devices[0];
            std::cout << "Using CPU Device : " << dev.getInfo<CL_DEVICE_NAME>() << "\n";
        }

        cl::Context context({ dev });

        cl::Program::Sources sources;
         
        tmp = false;

        if(model->mesh->vertexCount % localWorkSize != 0)
        {
            std::cout << std::endl << "Invalid Local Work Group Size. Falling back to 1." << std::endl;
            localWorkSize = 1;
        }

        std::string kernelSrc = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\kernels\\advanced_erosion.cl", &tmp);

        // Some constants
        kernelSrc = std::regex_replace(kernelSrc, std::regex("LOCAL_WORK_SIZE"), std::to_string(localWorkSize));

        sources.push_back({ kernelSrc.c_str(), kernelSrc.length() });

        cl::Program program(context, sources);
        if (program.build({ dev }) != CL_SUCCESS)
        {
            std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev) << std::endl;
            return;
        }

        cl::Buffer bufferD(context, CL_MEM_READ_WRITE, sizeof(Vert) * model->mesh->vertexCount);    


        cl::CommandQueue queue(context, dev);
        queue.enqueueWriteBuffer(bufferD, CL_TRUE, 0, sizeof(Vert) * model->mesh->vertexCount, model->mesh->vert);

        cl::Kernel kernel(program, "erode");

        kernel.setArg(0, bufferD);

        std::cout << std::endl << "Starting processing erosion." << std::endl;

        cl::NDRange global(model->mesh->vertexCount / localWorkSize);
        cl::NDRange local(localWorkSize);
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);

        queue.finish();

        std::cout << std::endl << "Finished processing erosion." << std::endl;

        queue.enqueueReadBuffer(bufferD, CL_TRUE, 0, sizeof(Vert) * model->mesh->vertexCount, model->mesh->vert);

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
 