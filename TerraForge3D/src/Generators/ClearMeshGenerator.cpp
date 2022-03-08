#include "ClearMeshGenerator.h"
#include "Utils.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"
ClearMeshGenerator::ClearMeshGenerator(ApplicationState *as, ComputeKernel *kernels)
{
	bool tmp = false;
	appState = as;
}

void ClearMeshGenerator::Generate(ComputeKernel *kernels)
{
	START_PROFILER();

	if(useGPU)
	{
		if (appState->mode == ApplicationMode::TERRAIN)
		{
			kernels->SetKernelArg("clear_mesh_terrain", 0, "mesh");
			kernels->ExecuteKernel("clear_mesh_terrain", cl::NDRange(1), cl::NDRange(appState->models.coreTerrain->mesh->vertexCount));
		}

		else if (appState->mode == ApplicationMode::CUSTOM_BASE)
		{
			kernels->SetKernelArg("clear_mesh_custom_base", 0, "mesh");
			kernels->SetKernelArg("clear_mesh_custom_base", 1, "mesh_copy");
			kernels->ExecuteKernel("clear_mesh_custom_base", cl::NDRange(1), cl::NDRange(appState->models.customBase->mesh->vertexCount));
		}
	}

	else
	{
		if (appState->mode == ApplicationMode::TERRAIN)
		{
			Mesh *mes = appState->models.coreTerrain->mesh;
			int vc = mes->vertexCount;

			for(int i=0; i<vc; i++)
			{
				mes->vert[i].normal.x = mes->vert[i].normal.y = mes->vert[i].normal.z = mes->vert[i].position.y = 0.0f;
				mes->vert[i].extras1.x = 0.0f;
				mes->vert[i].extras1.y = 0.0f;
				mes->vert[i].extras1.z = 0.0f;
			}
		}

		else if (appState->mode == ApplicationMode::CUSTOM_BASE)
		{
			Mesh *mes = appState->models.customBase->mesh;
			Mesh *mesC = appState->models.customBaseCopy->mesh;
			int vc = mesC->vertexCount;

			for(int i=0; i<vc; i++)
			{
				mes->vert[i].normal.x = mes->vert[i].normal.y = mes->vert[i].normal.z = 0.0f;
				mes->vert[i].position = mesC->vert[i].position;
				mes->vert[i].extras1.x = 0.0f;
				mes->vert[i].extras1.y = 0.0f;
				mes->vert[i].extras1.z = 0.0f;
			}
		}
	}

	END_PROFILER(time);
}

nlohmann::json ClearMeshGenerator::Save()
{
	nlohmann::json data;
	data["uiActive"] = uiActive;
	data["useGPU"] = useGPU;
	return data;
}

void ClearMeshGenerator::Load(nlohmann::json data)
{
	uiActive = data["uiActive"];
	useGPU = data["useGPU"];
}

void ClearMeshGenerator::ShowSettings()
{
	if(ImGui::Checkbox("Use GPU##CMG", &useGPU))
	{
	}

	ImGui::Checkbox("Use GPU For Normals(Flat Shading)##CMG", &appState->states.useGPUForNormals);
	ImGui::Text("Time : %lf ms", time);
}
