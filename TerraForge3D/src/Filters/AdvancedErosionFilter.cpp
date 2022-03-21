#include "Base/OpenCL/ComputeKernel.h"
#include "Filters/AdvancedErosionFilter.h"

#include "Data/ApplicationState.h"

#include "Utils/Utils.h"
#include "Platform.h"

#include <imgui.h>
#include <iostream>
#include <regex>


#define MAX(x, y) x>y?x:y
#define MIN(x, y) x<y?x:y

AdvancedErosionFilter::AdvancedErosionFilter(ApplicationState *appState)
	: Filter(appState, "Wind Erosion Filter (GPU)")
{
}

void AdvancedErosionFilter::Render()
{
	ImGui::DragInt("Local Work Group Size##windErosionFilter", &localWorkSize, 1, 1);
	ImGui::DragInt("Num Particles##WERO", &particles, 1, 100);
	ImGui::DragFloat("DT##WERO", &particle.dt, 0.01f);
	ImGui::DragFloat("Suspension##WERO", &particle.suspension, 0.0001f);
	ImGui::DragFloat("Abrasion##WERO", &particle.abrasion, 0.0001f);
	ImGui::DragFloat("Roughness##WERO", &particle.roughness, 0.01f);
	ImGui::DragFloat("Settling##WERO", &particle.settling, 0.01f);
	ImGui::DragFloat("Sediment##WERO", &particle.sediment, 0.01f);
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
	Model *model;

	if(appState->mode == ApplicationMode::TERRAIN)
	{
		model = appState->models.coreTerrain;
	}

	else if(appState->mode == ApplicationMode::CUSTOM_BASE)
	{
		model = appState->models.customBase;
	}

	else
	{
		return;
	}

	srand(time(NULL));
	model->mesh->RecalculateNormals();
	bool tmp = false;

	try
	{
		tmp = false;

		if(particles % localWorkSize != 0)
		{
			std::cout << std::endl << "Invalid Local Work Group Size. Falling back to 1." << std::endl;
			localWorkSize = 1;
		}

		std::string kernelSrc = ReadShaderSourceFile(GetExecutableDir() +
				PATH_SEPARATOR "Data" PATH_SEPARATOR "kernels"
				PATH_SEPARATOR "advanced_erosion.cl", &tmp);
		// Some constants
		kernelSrc = std::regex_replace(kernelSrc, std::regex("LOCAL_WORK_SIZE"), std::to_string(localWorkSize));
		kernels.Clear();
		kernels.AddSoruce(kernelSrc);
		kernels.BuildProgram("-I\"" + GetExecutableDir() + PATH_SEPARATOR
				"Data" PATH_SEPARATOR "kernels" + "\"");
		kernels.AddKernel("erode");
		kernels.CreateBuffer("mesh", CL_MEM_READ_WRITE, sizeof(Vert) * model->mesh->vertexCount);
		kernels.WriteBuffer("mesh", true, sizeof(Vert) * model->mesh->vertexCount, model->mesh->vert);
		particle.seed = (float)rand();
		particle.dimX = particle.dimY = model->mesh->res;
		kernels.CreateBuffer("part", CL_MEM_READ_WRITE, sizeof(WindErosionParticle));
		kernels.WriteBuffer("part", true, sizeof(WindErosionParticle), &particle);
		kernels.SetKernelArg("erode", 0, "mesh");
		kernels.SetKernelArg("erode", 1, "part");
		std::cout << std::endl << "Starting processing erosion." << std::endl;
		cl::NDRange global(particles);
		cl::NDRange local(localWorkSize);
		kernels.ExecuteKernel("erode", local, global);
		std::cout << "Finished processing erosion." << std::endl;
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
