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
	// LoadKernels();
}

MeshGeneratorManager::~MeshGeneratorManager()
{
}

void MeshGeneratorManager::ShowSettings()
{
	if (windowStat)
	{
		ImGui::Begin("Mesh Generators", &windowStat);
		ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
		ImGui::Text("Base Mesh Generators");
		ImGui::PopFont();

		ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
		ImGui::Text("GPU Noise Layer Generators");
		ImGui::PopFont();

		for (int i = 0; i < gpuNoiseLayers.size(); i++)
		{
			if (ImGui::CollapsingHeader((gpuNoiseLayers[i]->name + "##GPUNL" + std::to_string(i)).c_str()))
			{
				appState->workManager->SetRequireRemesh(gpuNoiseLayers[i]->ShowSetting(i));
				if (ImGui::Button(("Delete##GPUNOUSELAYER" + std::to_string(i)).c_str()))
				{
					appState->workManager->WaitForFinish();
					gpuNoiseLayers.erase(gpuNoiseLayers.begin() + i);
					break;
				}
			}
			ImGui::Separator();
		}

		if (ImGui::Button("Add##GPUNL"))
		{
			appState->workManager->WaitForFinish();
			gpuNoiseLayers.push_back(new GPUNoiseLayerGenerator(appState));
		}

		ImGui::Separator();
		ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
		ImGui::Text("CPU Noise Layer Generators");
		ImGui::PopFont();

		for (int i = 0; i < cpuNoiseLayers.size(); i++)
		{
			if (ImGui::CollapsingHeader((cpuNoiseLayers[i]->name + "##CPUNL" + std::to_string(i)).c_str()))
			{
				appState->workManager->SetRequireRemesh(cpuNoiseLayers[i]->ShowSetting(i));
				if (ImGui::Button(("Delete##CPUNOUSELAYER" + std::to_string(i)).c_str()))
				{
					appState->workManager->WaitForFinish();
					cpuNoiseLayers.erase(cpuNoiseLayers.begin() + i);
					break;
				}
			}
			ImGui::Separator();
		}

		if (ImGui::Button("Add##CPULAYER"))
		{
			appState->workManager->WaitForFinish();
			cpuNoiseLayers.push_back(new CPUNoiseLayersGenerator(appState));
		}

		

		ImGui::Separator();
		ImGui::PushFont(GetUIFont("OpenSans-Semi-Bold"));
		ImGui::Text("CPU Node Editor Generators");
		ImGui::PopFont();

		for (int i = 0; i < cpuNodeEditors.size(); i++)
		{
			if (ImGui::CollapsingHeader((cpuNodeEditors[i]->name + "##CPUNE" + std::to_string(i)).c_str()))
			{
				appState->workManager->SetRequireRemesh(cpuNodeEditors[i]->ShowSetting(i));
				if (ImGui::Button(("Delete##CPNE" + std::to_string(i)).c_str()))
				{
					appState->workManager->WaitForFinish();
					cpuNodeEditors.erase(cpuNodeEditors.begin() + i);
					break;
				}
			}

			ImGui::Separator();
		}

		if (ImGui::Button("Add##CPUNE"))
		{
			appState->workManager->WaitForFinish();
			cpuNodeEditors.push_back(new CPUNodeEditor(appState));
		}

		ImGui::Separator();
		ImGui::Text("Time : %lf ms", time);
		ImGui::End();
	}

	for (int i = 0; i < gpuNoiseLayers.size(); i++) appState->workManager->SetRequireRemesh(gpuNoiseLayers[i]->Update());
	for (int i = 0; i < cpuNoiseLayers.size(); i++) appState->workManager->SetRequireRemesh(cpuNoiseLayers[i]->Update());
	for (int i = 0; i < cpuNodeEditors.size(); i++) appState->workManager->SetRequireRemesh(cpuNodeEditors[i]->Update());
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
	appState->workManager->WaitForFinish();

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
		//gpuNoiseLayers.push_back(new GPUNoiseLayerGenerator(appState, kernels));
		//gpuNoiseLayers.back()->Load(data["gpunl"][i]);
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
