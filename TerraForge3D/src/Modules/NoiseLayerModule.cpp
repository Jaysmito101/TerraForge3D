#include "NoiseLayerModule.h"
#include "ModuleManager.h"
#include "imgui.h"

NoiseLayerModule::NoiseLayerModule(std::string id, ModuleManager* manager)
	:Module(id, manager)
{
	type = ModuleData::Type::UIModule;
	functions["Render"] = manager->GetFunctionPointer(id, "RenderNoiseLayer");
	functions["Evaluate"] = manager->GetFunctionPointer(id, "EvaluateNoiseLayer");
	functions["GetNoiseLayerName"] = manager->GetFunctionPointer(id, "GetNoiseLayerName");
	functions["Save"] = manager->GetFunctionPointer(id, "SaveNoiseLayerData");
	functions["Load"] = manager->GetFunctionPointer(id, "LoadNoiseLayerData");
	functions["LoadNoiseLayer"] = manager->GetFunctionPointer(id, "LoadNoiseLayer");
	name = ((char*(*)(void))(functions["GetNoiseLayerName"]))();
}

void NoiseLayerModule::Render()
{
	((void(*)(void*))(functions["Render"]))(ImGui::GetCurrentContext());
}

nlohmann::json NoiseLayerModule::Save()
{
	return nlohmann::json::parse(std::string( ((char* (*)(void))functions["Save"])() ));
}

void NoiseLayerModule::Load(nlohmann::json data)
{
	((void (*)(char*))functions["Load"])(data.dump(4).data());
}

float NoiseLayerModule::Evaluate(float x, float y, float z)
{
	return ((float (*)(float, float, float))functions["Evaluate"])(x, y, z);
}
