#include "CPUNoiseLayersGenerator.h"

#include "Utils.h"
#include "Profiler.h"
#include "Data/ApplicationState.h"

int count = 0;

CPUNoiseLayersGenerator::CPUNoiseLayersGenerator(ApplicationState* as)
{
	uid = GenerateId(32);
	name.reserve(1024);
	name = "CPU Noise Layer " + std::to_string(count++);
	appState = as;
	noiseManager = new LayeredNoiseManager();
}

CPUNoiseLayersGenerator::~CPUNoiseLayersGenerator()
{
	delete noiseManager;
}

nlohmann::json CPUNoiseLayersGenerator::Save()
{
	nlohmann::json data;
	data["name"] = name;
	data["uid"] = uid;
	data["wind"] = windowStat;
	data["noise"] = noiseManager->Save();
	data["uiActive"] = uiActive;
	data["enabled"] = enabled;
	return data;
}

void CPUNoiseLayersGenerator::Load(nlohmann::json data)
{
	name = data["name"];
	uid = data["uid"];
	windowStat = data["wind"];
	uiActive = data["uiActive"];
	enabled = data["enabled"];
	noiseManager->Load(data["noise"]);
}

void CPUNoiseLayersGenerator::ShowSetting(int id)
{
	ImGui::InputText(("Name##CPUNL" + std::to_string(id)).c_str(), name.data(), 1024);
	ImGui::Checkbox(("Enabled##CPUNL" + std::to_string(id)).c_str(), &enabled);
	if (ImGui::Button(("Edit##CPUNL" + std::to_string(id)).c_str()))
	{
		windowStat = true;
	}
	ImGui::Text("UID : %s", uid.data());
	ImGui::Text("Time : %lf ms", time);
}

void CPUNoiseLayersGenerator::Update()
{
	if (windowStat)
	{
		ImGui::Begin((name + "##" + uid).c_str(), &windowStat);
		noiseManager->Render();
		ImGui::End();
	}
	if (!appState->states.remeshing)
	{
		noiseManager->UpdateLayers();
	}
}

float CPUNoiseLayersGenerator::EvaluateAt(float x, float y, float z)
{
	return noiseManager->Evaluate(x, y, z);
}
