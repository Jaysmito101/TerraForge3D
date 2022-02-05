#include "MeshGeneratorManager.h"

#include "Utils.h"
#include "Base/Base.h"
#include "Data/ApplicationState.h"

#include "ClearMeshGenerator.h"
#include "CPUNoiseLayersGenerator.h"
#include "GPUNoiseLayerGenerator.h"

#include "Profiler.h"

MeshGeneratorManager::MeshGeneratorManager(ApplicationState* as)
	:appState(as)
{
	isRemeshing = &appState->states.remeshing;
	kernels = new ComputeKernel();
	clearMeshGen = new ClearMeshGenerator(appState, kernels);
	LoadKernels();
}

MeshGeneratorManager::~MeshGeneratorManager()
{
	delete clearMeshGen;
	delete kernels;
}

void MeshGeneratorManager::Generate()
{
	if (*isRemeshing)
		return;
	

	if (appState->mode == ApplicationMode::TERRAIN)
	{
		appState->models.coreTerrain->UploadToGPU();
	}
	else if (appState->mode == ApplicationMode::CUSTOM_BASE)
	{
		appState->models.customBase->UploadToGPU();
	}

	*isRemeshing = true;
	std::thread worker([this] {
		START_PROFILER();
		ExecuteKernels();
		ExecuteCPUGenerators();

		if (appState->mode == ApplicationMode::TERRAIN)
		{
			appState->models.coreTerrain->mesh->RecalculateNormals();
		}
		else if (appState->mode == ApplicationMode::CUSTOM_BASE)
		{
			appState->models.customBase->mesh->RecalculateNormals();
		}
		END_PROFILER(time);
		*isRemeshing = false;
		});
	worker.detach();
}

void MeshGeneratorManager::GenerateSync()
{
	while (appState->states.remeshing);

	*isRemeshing = true;

	ExecuteKernels();
	ExecuteCPUGenerators();

	if (appState->mode == ApplicationMode::TERRAIN)
	{
		appState->models.coreTerrain->mesh->RecalculateNormals();
		appState->models.coreTerrain->UploadToGPU();
	}
	else if (appState->mode == ApplicationMode::CUSTOM_BASE)
	{
		appState->models.customBase->mesh->RecalculateNormals();
		appState->models.customBase->UploadToGPU();
	}

	*isRemeshing = false;

}

void MeshGeneratorManager::ShowSettings()
{
	if (windowStat)
	{
		ImGui::Begin("Mesh Generators", &windowStat);

		ImGui::Text("Base Mesh Generators");
		if (ImGui::CollapsingHeader("Auto Base Mesh Gen (GPU)", clearMeshGen->uiActive))
		{
			clearMeshGen->ShowSettings();
		}
		ImGui::Separator();

		ImGui::Text("CPU Noise Layer Generators");
		for (int i = 0; i < cpuNoiseLayers.size(); i++)
		{
			if (ImGui::CollapsingHeader((cpuNoiseLayers[i]->name + "##CPUNL" + std::to_string(i)).c_str(), cpuNoiseLayers[i]->uiActive))
			{
				cpuNoiseLayers[i]->ShowSetting(i);
				if (ImGui::Button(("Delete##CPUNOUSELAYER" + std::to_string(i)).c_str()))
				{
					while (*isRemeshing);
					cpuNoiseLayers.erase(cpuNoiseLayers.begin() + i);
					break;
				}
			}
			ImGui::Separator();
		}
		if (ImGui::Button("Add##CPULAYER"))
		{
			while (*isRemeshing);
			cpuNoiseLayers.push_back(new CPUNoiseLayersGenerator(appState));
		}
		ImGui::Separator();

		ImGui::Text("GPU Noise Layer Generators");
		for (int i = 0; i < gpuNoiseLayers.size(); i++)
		{
			if (ImGui::CollapsingHeader((gpuNoiseLayers[i]->name + "##GPUNL" + std::to_string(i)).c_str(), gpuNoiseLayers[i]->uiActive))
			{
				gpuNoiseLayers[i]->ShowSetting(i);
				if (ImGui::Button(("Delete##GPUNOUSELAYER" + std::to_string(i)).c_str()))
				{
					while (*isRemeshing);
					gpuNoiseLayers.erase(gpuNoiseLayers.begin() + i);
					break;
				}
			}
			ImGui::Separator();
		}
		if (ImGui::Button("Add##GPUNL"))
		{
			while (*isRemeshing);
			gpuNoiseLayers.push_back(new GPUNoiseLayerGenerator(appState, kernels));
		}
		ImGui::Separator();

		ImGui::Text("Time : %lf ms", time);
		

		ImGui::End();
	}

	for (int i = 0; i < cpuNoiseLayers.size(); i++)
	{
		cpuNoiseLayers[i]->Update();
	}

	for (int i = 0; i < gpuNoiseLayers.size(); i++)
	{
		gpuNoiseLayers[i]->Update();
	}
}

void MeshGeneratorManager::GenerateForTerrain()
{
	int res = appState->globals.resolution;
	float scale = appState->globals.scale;
	Model* mod = appState->models.coreTerrain;
	for (int y = 0; y < res; y++)
	{
		for (int x = 0; x < res; x++)
		{
			float elev = 0.0f;
			for (int i = 0; i < cpuNoiseLayers.size(); i++)
			{
				if (cpuNoiseLayers[i]->enabled)
					elev += cpuNoiseLayers[i]->EvaluateAt(x, y, 0);
			}
			mod->mesh->AddElevation(elev, x, y);
		}
	}
}

void MeshGeneratorManager::GenerateForCustomBase()
{
	Model* customModel = appState->models.customBase;
	Model* customModelCopy = appState->models.customBaseCopy;
	float scale = appState->globals.scale;
	appState->stats.vertexCount = customModel->mesh->vertexCount;
	for (int i = 0; i < customModel->mesh->vertexCount; i++)
	{
		Vert tmp = customModelCopy->mesh->vert[i];
		float x = tmp.position.x;
		float y = tmp.position.y;
		float z = tmp.position.z;
		float elev = 0.0f;
		for (int i = 0; i < cpuNoiseLayers.size(); i++)
		{
			if(cpuNoiseLayers[i]->enabled)
				elev += cpuNoiseLayers[i]->EvaluateAt(x, y, z);
		}
		tmp.position *= scale;
		tmp.position += elev * tmp.normal;
		customModel->mesh->vert[i].extras1.x = elev;
		customModel->mesh->vert[i].position = tmp.position;
	}
}

void MeshGeneratorManager::ExecuteKernels()
{
	if (appState->mode == ApplicationMode::TERRAIN)
	{

		if (appState->models.coreTerrain->mesh->res != appState->globals.resolution || appState->models.coreTerrain->mesh->sc != appState->globals.scale)
		{
			appState->models.coreTerrain->mesh->res = appState->globals.resolution;
			appState->models.coreTerrain->mesh->sc = appState->globals.scale;
			appState->models.coreTerrain->mesh->GeneratePlane(appState->globals.resolution, appState->globals.scale);
		}

		kernels->CreateBuffer("mesh", CL_MEM_READ_WRITE, appState->models.coreTerrain->mesh->vertexCount * sizeof(Vert));
		kernels->WriteBuffer("mesh", true, appState->models.coreTerrain->mesh->vertexCount * sizeof(Vert), appState->models.coreTerrain->mesh->vert);

		clearMeshGen->Generate(kernels);

		for (int i = 0; i < gpuNoiseLayers.size(); i++)
		{
			if (gpuNoiseLayers[i]->enabled)
				gpuNoiseLayers[i]->Generate(kernels);
		}

		kernels->ReadBuffer("mesh", true, appState->models.coreTerrain->mesh->vertexCount * sizeof(Vert), appState->models.coreTerrain->mesh->vert);

	}
	else if (appState->mode == ApplicationMode::CUSTOM_BASE)
	{
		kernels->CreateBuffer("mesh", CL_MEM_READ_WRITE, appState->models.customBase->mesh->vertexCount * sizeof(Vert));
		kernels->WriteBuffer("mesh", true, appState->models.customBase->mesh->vertexCount * sizeof(Vert), appState->models.customBase->mesh->vert);

		kernels->CreateBuffer("mesh_copy", CL_MEM_READ_WRITE, appState->models.customBase->mesh->vertexCount * sizeof(Vert));
		kernels->WriteBuffer("mesh_copy", true, appState->models.customBase->mesh->vertexCount * sizeof(Vert), appState->models.customBaseCopy->mesh->vert);

		clearMeshGen->Generate(kernels);

		kernels->ReadBuffer("mesh", true, appState->models.customBase->mesh->vertexCount * sizeof(Vert), appState->models.customBase->mesh->vert);
	}
}

void MeshGeneratorManager::ExecuteCPUGenerators()
{
	if (appState->mode == ApplicationMode::TERRAIN)
	{
		GenerateForTerrain();
	}
	else if (appState->mode == ApplicationMode::CUSTOM_BASE)
	{
		GenerateForCustomBase();
	}
} 

void MeshGeneratorManager::LoadKernels()
{
	bool tmp = false;
	std::string source = ReadShaderSourceFile(GetExecutableDir() + "\\Data\\kernels\\generators\\generators.cl", &tmp);
	kernels->AddSoruce(source);
	kernels->BuildProgram("-I" + appState->globals.kernelsIncludeDir);
	kernels->AddKernel("clear_mesh_terrain");
	kernels->AddKernel("clear_mesh_custom_base");
	kernels->AddKernel("noise_layer_terrain");
	kernels->AddKernel("noise_layer_custom_base");
} 

nlohmann::json MeshGeneratorManager::Save()
{
	nlohmann::json data, tmp;
	data["cmg"] = clearMeshGen->Save();
	for (int i = 0; i < cpuNoiseLayers.size(); i++)
	{
		tmp.push_back(cpuNoiseLayers[i]->Save());
	}
	data["cpunl"] = tmp;
	tmp = nlohmann::json();
	for (int i = 0; i < gpuNoiseLayers.size(); i++)
	{
		tmp.push_back(gpuNoiseLayers[i]->Save());
	}
	data["gpunl"] = tmp;
	return data;
}

void MeshGeneratorManager::Load(nlohmann::json data)
{
	while (*isRemeshing);
	clearMeshGen->Load(data["cmg"]);

	for (int i = 0; i < cpuNoiseLayers.size(); i++)
		delete cpuNoiseLayers[i];

	cpuNoiseLayers.clear();
	for (int i = 0; i < data["cpunl"].size(); i++)
	{
		cpuNoiseLayers.push_back(new CPUNoiseLayersGenerator(appState));
		cpuNoiseLayers.back()->Load(data[i]);
	}

	for (int i = 0; i < gpuNoiseLayers.size(); i++)
		delete gpuNoiseLayers[i];

	gpuNoiseLayers.clear();
	for (int i = 0; i < data["gpunl"].size(); i++)
	{
		gpuNoiseLayers.push_back(new GPUNoiseLayerGenerator(appState, kernels));
		gpuNoiseLayers.back()->Load(data[i]);
	}

}
