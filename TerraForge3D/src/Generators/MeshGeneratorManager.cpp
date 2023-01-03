#include "Generators/MeshGeneratorManager.h"

#include "Utils/Utils.h"
#include "Base/Base.h"
#include "Data/ApplicationState.h"

#include "Generators/CPUNoiseLayersGenerator.h"
#include "Generators/GPUNoiseLayerGenerator.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include "Base/UIFontManager.h"

#include "Profiler.h"
#include "Platform.h"

MeshGeneratorManager::MeshGeneratorManager(ApplicationState* as)
	:appState(as)
{
	isRemeshing = &appState->states.remeshing;
	kernels = new ComputeKernel();
	cpuWorkerCount = max(1, std::thread::hardware_concurrency());
	LoadKernels();
	for (int i = 0; i < CPU_GENERATOR_WORKER_COUNT_MAX; i++)
	{
		cpuGeneratorWorkers[i] = new CPUGeneratorWorker(as, i);
		cpuGeneratorWorkers[i]->Start();
	}
}

MeshGeneratorManager::~MeshGeneratorManager()
{
	for (int i = 0; i < CPU_GENERATOR_WORKER_COUNT_MAX; i++) cpuGeneratorWorkers[i]->Quit();
	delete kernels;
	for (int i = 0; i < CPU_GENERATOR_WORKER_COUNT_MAX; i++) delete cpuGeneratorWorkers[i];
}

void MeshGeneratorManager::Generate()
{
	if (*isRemeshing) return;
	if (appState->states.pauseUpdation) return;

	for (int i = 0; i < 6; i++) appState->mainMap.currentTileDataLayers[i]->UploadToGPU();

	*isRemeshing = true;
	std::thread worker([this]
		{
			START_PROFILER();

			ExecuteKernels();
			ExecuteCPUGenerators();

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
	appState->mainMap.currentTileDataLayers[0]->UploadToGPU();
	*isRemeshing = false;
}

void MeshGeneratorManager::ShowSettings()
{
	if (windowStat)
	{
		ImGui::Begin("Mesh Generators", &windowStat);
		ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
		ImGui::Text("Base Mesh Generators");
		ImGui::PopFont();


		//if (ImGui::CollapsingHeader("Auto Base Mesh Gen (GPU)", clearMeshGen->uiActive))
		//{
		//	clearMeshGen->ShowSettings();
		//}

		if (ImGui::CollapsingHeader("CPU Worker Threads"))
		{
			ImGui::DragInt("Thread Count#CPUMeshGen", &this->cpuWorkerCount, 0.01f, 1, CPU_GENERATOR_WORKER_COUNT_MAX);
			this->cpuWorkerCount = std::clamp(this->cpuWorkerCount, 1, CPU_GENERATOR_WORKER_COUNT_MAX);
			this->appState->globals.cpuWorkerThreadsActive = this->cpuWorkerCount;
			for (int i = 0; i < CPU_GENERATOR_WORKER_COUNT_MAX; i++)
			{
				ImGui::Text("[%d] (%f)", i, cpuGeneratorWorkers[i]->GetJobTime());
				ImGui::SameLine();
				if (cpuGeneratorWorkers[i]->IsIdle()) ImGui::Button("Idle");
				else ImGui::ProgressBar(cpuGeneratorWorkers[i]->GetProgress());
				ImGui::Separator();
			}
		}

		ImGui::Separator();
		ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
		ImGui::Text("CPU Noise Layer Generators");
		ImGui::PopFont();

		for (int i = 0; i < cpuNoiseLayers.size(); i++)
		{
			if (ImGui::CollapsingHeader((cpuNoiseLayers[i]->name + "##CPUNL" + std::to_string(i)).c_str()))
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
		ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
		ImGui::Text("GPU Noise Layer Generators");
		ImGui::PopFont();

		for (int i = 0; i < gpuNoiseLayers.size(); i++)
		{
			if (ImGui::CollapsingHeader((gpuNoiseLayers[i]->name + "##GPUNL" + std::to_string(i)).c_str()))
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
		ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
		ImGui::Text("CPU Node Editor Generators");
		ImGui::PopFont();

		for (int i = 0; i < cpuNodeEditors.size(); i++)
		{
			if (ImGui::CollapsingHeader((cpuNodeEditors[i]->name + "##CPUNE" + std::to_string(i)).c_str()))
			{
				cpuNodeEditors[i]->ShowSetting(i);

				if (ImGui::Button(("Delete##CPNE" + std::to_string(i)).c_str()))
				{
					while (*isRemeshing);

					cpuNodeEditors.erase(cpuNodeEditors.begin() + i);
					break;
				}
			}

			ImGui::Separator();
		}

		if (ImGui::Button("Add##CPUNE"))
		{
			while (*isRemeshing);

			cpuNodeEditors.push_back(new CPUNodeEditor(appState));
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

	for (int i = 0; i < cpuNodeEditors.size(); i++)
	{
		cpuNodeEditors[i]->Update();
	}
}

void MeshGeneratorManager::GenerateForTerrain()
{

	int eachSize = appState->mainMap.tileResolution / cpuWorkerCount;

	for (int i = 0; i < cpuWorkerCount; i++)
	{
		cpuGeneratorWorkers[i]->AddJob(cpuNoiseLayers, cpuNodeEditors, eachSize * i, eachSize);
	}

	for (int i = 0; i < cpuWorkerCount; i++)
	{
		cpuGeneratorWorkers[i]->WaitForFinish();
	}


}


void MeshGeneratorManager::ExecuteKernels()
{
	/*
	if (appState->models.coreTerrain->mesh->res != appState->globals.resolution || appState->models.coreTerrain->mesh->sc != appState->globals.scale)
	{
		appState->models.coreTerrain->mesh->res = appState->globals.resolution;
		appState->models.coreTerrain->mesh->sc = appState->globals.scale;
		appState->models.coreTerrain->mesh->GeneratePlane(appState->globals.resolution, appState->globals.scale);
	}

	kernels->CreateBuffer("mesh", CL_MEM_READ_WRITE, appState->models.coreTerrain->mesh->vertexCount * sizeof(Vert));
	kernels->WriteBuffer("mesh", true, appState->models.coreTerrain->mesh->vertexCount * sizeof(Vert), appState->models.coreTerrain->mesh->vert);

	if (clearMeshGen->useGPU)
	{
		clearMeshGen->Generate(kernels);
	}

	for (int i = 0; i < gpuNoiseLayers.size(); i++)
	{
		if (gpuNoiseLayers[i]->enabled)
		{
			gpuNoiseLayers[i]->Generate(kernels);
		}
	}

	if (clearMeshGen->useGPUForNormals)
	{
		kernels->CreateBuffer("indices", CL_MEM_READ_WRITE, appState->models.coreTerrain->mesh->indexCount * sizeof(int));
		kernels->WriteBuffer("indices", true, appState->models.coreTerrain->mesh->indexCount * sizeof(int), appState->models.coreTerrain->mesh->indices);
		kernels->SetKernelArg("gen_normals", 0, "mesh");
		kernels->SetKernelArg("gen_normals", 1, "indices");
		int ls = 512;
		int ic = appState->models.coreTerrain->mesh->indexCount;

		while (ic % ls != 0)
		{
			ls /= 2;
		}

		kernels->ExecuteKernel("gen_normals", cl::NDRange(ls), cl::NDRange(ic));
		kernels->SetKernelArg("normalize_normals", 0, "mesh");
		kernels->ExecuteKernel("normalize_normals", cl::NDRange(1), cl::NDRange(appState->models.coreTerrain->mesh->vertexCount));
	}

	kernels->ReadBuffer("mesh", true, appState->models.coreTerrain->mesh->vertexCount * sizeof(Vert), appState->models.coreTerrain->mesh->vert);
	*/
}

void MeshGeneratorManager::ExecuteCPUGenerators()
{
	GenerateForTerrain();
}

void MeshGeneratorManager::LoadKernels()
{
	bool tmp = false;
	std::string source = ReadShaderSourceFile(GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "kernels" PATH_SEPARATOR "generators" PATH_SEPARATOR "generators.cl", &tmp);
	kernels->AddSoruce(source);
	kernels->BuildProgram("-I" + appState->globals.kernelsIncludeDir + " -cl-fast-relaxed-math -cl-mad-enable");
	kernels->AddKernel("clear_mesh_terrain");
	kernels->AddKernel("clear_mesh_custom_base");
	kernels->AddKernel("noise_layer_terrain");
	kernels->AddKernel("noise_layer_custom_base");
	kernels->AddKernel("gen_normals");
	kernels->AddKernel("normalize_normals");
}

nlohmann::json MeshGeneratorManager::Save()
{
	nlohmann::json data, tmp;
	//data["cmg"] = clearMeshGen->Save();

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
	tmp = nlohmann::json();

	for (int i = 0; i < cpuNodeEditors.size(); i++)
	{
		tmp.push_back(cpuNodeEditors[i]->Save());
	}

	data["cpune"] = tmp;
	return data;
}

void MeshGeneratorManager::Load(nlohmann::json data)
{
	while (*isRemeshing);

	//clearMeshGen->Load(data["cmg"]);

	for (int i = 0; i < cpuNoiseLayers.size(); i++)
	{
		delete cpuNoiseLayers[i];
	}

	cpuNoiseLayers.clear();

	for (int i = 0; i < data["cpunl"].size(); i++)
	{
		cpuNoiseLayers.push_back(new CPUNoiseLayersGenerator(appState));
		cpuNoiseLayers.back()->Load(data["cpunl"][i]);
	}

	for (int i = 0; i < gpuNoiseLayers.size(); i++)
	{
		delete gpuNoiseLayers[i];
	}

	gpuNoiseLayers.clear();

	for (int i = 0; i < data["gpunl"].size(); i++)
	{
		gpuNoiseLayers.push_back(new GPUNoiseLayerGenerator(appState, kernels));
		gpuNoiseLayers.back()->Load(data["gpunl"][i]);
	}

	for (int i = 0; i < cpuNodeEditors.size(); i++)
	{
		delete cpuNodeEditors[i];
	}

	cpuNodeEditors.clear();

	for (int i = 0; i < data["cpune"].size(); i++)
	{
		cpuNodeEditors.push_back(new CPUNodeEditor(appState));
		cpuNodeEditors.back()->Load(data["cpune"][i]);
	}
}
