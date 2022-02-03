#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
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

    ImGui::DragInt("Max Droplet Lifetime##erosionFilter", &maxDropletLifetime, 1, 1);
    ImGui::DragInt("Num Particles##erosionFilter", &numIterations, 1, 1);
    ImGui::DragInt("Erosoion Radius##erosionFilter", &erosionRadius, 1, 1);


    ImGui::DragFloat("Sediment Capacity Factor##erosionFilter", &sedimentCapacityFactor, 0.1f, 0);
    ImGui::DragFloat("Minimum Sediment Capacity##erosionFilter", &minSedimentCapacity, 0.1f, 0);
    ImGui::DragFloat("Inertia##erosionFilter", &inertia, 0.1f, 0);
    ImGui::DragFloat("Errode Speed##erosionFilter", &erodeSpeed, 0.1f, 0);
    ImGui::DragFloat("Evaporation Speed##erosionFilter", &evaporateSpeed, 0.1f, 0);
    ImGui::DragFloat("Deposition Speed##erosionFilter", &depositSpeed, 0.1f, 0);
    ImGui::DragFloat("Gravity##erosionFilter", &gravity, 0.1f, 0);



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
    int mapSize = model->mesh->vertexCount;
    float* map = new float[model->mesh->vertexCount];
    for (int i = 0; i < model->mesh->vertexCount; i++) {
        map[i] = model->mesh->vert[i].position.y;
    }

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

        std::string kernelSrc = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\kernels\\advanced_erosion.cl", &tmp);

        // Some constants
        kernelSrc = std::regex_replace(kernelSrc, std::regex("LOCAL_GROUP_SIZE"), "16");

        sources.push_back({ kernelSrc.c_str(), kernelSrc.length() });

        cl::Program program(context, sources);
        if (program.build({ dev }) != CL_SUCCESS)
        {
            std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev) << std::endl;
            return;
        }

        cl::Buffer bufferA(context, CL_MEM_READ_WRITE, sizeof(float) * mapSize);
        cl::Buffer bufferC(context, CL_MEM_READ_WRITE, sizeof(float) * mapSize);    
        cl::Buffer bufferN(context, CL_MEM_READ_WRITE, sizeof(int));


        cl::CommandQueue queue(context, dev);
        queue.enqueueWriteBuffer(bufferA, CL_TRUE, 0, sizeof(float) * mapSize, map);
        queue.enqueueWriteBuffer(bufferN, CL_TRUE, 0, sizeof(int), &model->mesh->res);

        cl::Kernel kernel(program, "erode");

        kernel.setArg(0, bufferA);
        kernel.setArg(1, bufferC);
        kernel.setArg(2, bufferN);

        std::cout << std::endl << "Starting processing erosion." << std::endl;

        cl::NDRange global(model->mesh->res, model->mesh->res);
        cl::NDRange local(1, 1);
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);

        queue.finish();

        std::cout << std::endl << "Finished processing erosion." << std::endl;

        queue.enqueueReadBuffer(bufferC, CL_TRUE, 0, sizeof(float) * mapSize, map);
    }
    catch (std::exception ex)
    {
        std::cout << "OpenCL Exception : " << ex.what() << "\n";
        return;
    }

    /*
    for (int i = 0; i < model->mesh->res; i++) {
        for (int j = 0; j < model->mesh->res; j++)
        {
            map[i * model->mesh->res + j] = sin(j * 0.05) * 5;
        }
    }
    */
	



    

    for (int i = 0; i < model->mesh->vertexCount; i++) {
        model->mesh->vert[i].position.y = map[i];
    }

    // Deletion 

    delete[] map;

	model->mesh->RecalculateNormals();
	model->UploadToGPU();
}
 