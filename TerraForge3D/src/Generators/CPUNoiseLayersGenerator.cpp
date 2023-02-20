#include "Generators/CPUNoiseLayersGenerator.h"

#include "Utils/Utils.h"
#include "Profiler.h"
#include "Data/ApplicationState.h"
#include "Base/OpenCL/OpenCLContext.h"

int count = 1;

CPUNoiseLayersGenerator::CPUNoiseLayersGenerator(ApplicationState *as)
{
	uid = GenerateId(32);
	name.reserve(1024);
	name = "CPU Noise Layer " + std::to_string(count++);
	appState = as;
	noiseManager = new LayeredNoiseManager();
	maskManager = new GeneratorMaskManager(nullptr, uid, as);
}

CPUNoiseLayersGenerator::~CPUNoiseLayersGenerator()
{
	delete noiseManager;
	delete maskManager;
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
	data["maskManager"] = maskManager->Save();
	return data;
}

void CPUNoiseLayersGenerator::Load(nlohmann::json data)
{
	name = data["name"];
	uid = data["uid"];
	windowStat = data["wind"];
	uiActive = data["uiActive"];
	enabled = data["enabled"];
	maskManager->Load(data["maskManager"]);
	noiseManager->Load(data["noise"]);
}

bool CPUNoiseLayersGenerator::ShowSetting(int id)
{
	bool stateChanged = false;
	ImGui::PushID(id);
	ImGui::InputText("Name##CPUNL", name.data(), 1024);
	stateChanged |= ImGui::Checkbox("Enabled##CPUNL", &enabled);
	stateChanged |= ShowLayerUpdationMethod("Set Method##CPUNL", &this->setMode);

	if (ImGui::Button("Edit Layers##CPUNL")) windowStat = true;


	if(ImGui::CollapsingHeader(("Custom Base Mask##GMSK" + uid).c_str() ) ) stateChanged |= maskManager->ShowSettings();

	ImGui::Separator();
	ImGui::Text("UID : %s", uid.data());
	ImGui::Text("Time : %lf ms", time);
	ImGui::PopID();
	ImGui::NewLine();
	
	return stateChanged;
}

bool CPUNoiseLayersGenerator::Update()
{
	bool stateChanged = false;
	if (windowStat)
	{
		ImGui::Begin((name + "##" + uid).c_str(), &windowStat);
		stateChanged = noiseManager->Render();
		ImGui::End();
	}
	if (!appState->workManager->IsWorking()) noiseManager->UpdateLayers();
	return stateChanged;
}

float CPUNoiseLayersGenerator::EvaluateAt(float x, float y, float z) const
{
	float result = 0.0f, pResult = y;
	y = 0.0f;

	if(maskManager->enabled)
	{
		result = maskManager->EvaluateAt(x, y, z, noiseManager->Evaluate(x + noiseManager->offset[0], y + noiseManager->offset[1], z + noiseManager->offset[2]));
	}
	else
	{
		result = noiseManager->Evaluate(x, y, z);
	}

	return UpdateLayerWithUpdateMethod(pResult, result, this->setMode);
}
