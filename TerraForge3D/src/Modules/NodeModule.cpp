#include "NodeModule.h"
#include "ModuleManager.h"
#include "imgui.h"

#include "Base/NodeEditor/NodeEditor.h"

NodeModule::NodeModule(std::string id, ModuleManager* manager)
	:Module(id, manager)
{
	type = ModuleData::Type::UIModule;
	functions["Render"] = manager->GetFunctionPointer(id, "RenderNode");
	functions["Evaluate"] = manager->GetFunctionPointer(id, "EvaluateNode");
	functions["GetNodeName"] = manager->GetFunctionPointer(id, "GetNodeName");
	functions["Save"] = manager->GetFunctionPointer(id, "SaveNodeData");
	functions["Load"] = manager->GetFunctionPointer(id, "LoadNodeData");
	functions["LoadNode"] = manager->GetFunctionPointer(id, "LoadNode");
	name = ((char*(*)(void))(functions["GetNodeName"]))();
}

void NodeModule::Render()
{
	((void(*)(void*))(functions["Render"]))(ImGui::GetCurrentContext());
}

nlohmann::json NodeModule::Save()
{
	return nlohmann::json::parse(std::string( ((char* (*)(void))functions["Save"])() ));
}

void NodeModule::Load(nlohmann::json data)
{
	((void (*)(char*))functions["Load"])(data.dump(4).data());
}

float NodeModule::Evaluate(NodeInputParam input)
{
	return ((float (*)(NodeInputParam))functions["Evaluate"])(input);
}
