#include "Defines.h"
#ifdef NOISE_LAYER_MODULE

#include "NoiseLayerModuleExport.h"
#include "NoiseLayerModuleInterface.h"
#include "json.hpp"
#include "imgui.h"

void LoadNoiseLayer()
{
	OnNoiseLayerLoad();
}

void RenderNoiseLayer(void* imguiContext)
{
	ImGui::SetCurrentContext((ImGuiContext*)imguiContext);
	OnNoiseLayerRender();
}

float EvaluateNoiseLayer(float x, float y, float z)
{
	return OnNoiseLayerEvaluate(x, y, z);
}

char* GetNoiseLayerName()
{
	std::string dataS = NoiseLayerName();
	char* data = (char*)malloc(dataS.size());
	memcpy(data, dataS.data(), dataS.size());
	return data;
}

char* SaveNoiseLayerData()
{
	nlohmann::json dataJ = OnNoiseLayerDataSave();
	std::string dataS = dataJ.dump(4);
	char* data = (char*)malloc(dataS.size());
	memcpy(data, dataS.data(), dataS.size());
	return data;
}

void LoadNoiseLayerData(char* data)
{
	std::string dataS = data;
	nlohmann::json dataJ = nlohmann::json::parse(dataS);
	OnNoiseLayerDataLoad(dataJ);
}

#endif