#include "Defines.h"
#ifdef NODE_MODULE

#include "NodeModuleExport.h"
#include "NodeModuleInterface.h"
#include "BaseModuleInterface.h"
#include "json.hpp"
#include "imgui.h"

void LoadNode()
{
	OnNodeLoad();
}

void RenderNode(void* imguiContext)
{
	ImGui::SetCurrentContext((ImGuiContext*)imguiContext);
	OnNodeRender();
}

float EvaluateNode(NodeInputParam input)
{
	NodeOutput out = OnNodeEvaluate(input);
	return out.value;
}

char* GetNodeName()
{
	std::string dataS = GetName();
	char* data = (char*)malloc(dataS.size());
	memcpy(data, dataS.data(), dataS.size());
	return data;
}

char* SaveNodeData()
{
	nlohmann::json dataJ = OnNodeDataSave();
	std::string dataS = dataJ.dump(4);
	char* data = (char*)malloc(dataS.size());
	memcpy(data, dataS.data(), dataS.size());
	return data;
}

void LoadNodeData(char* data)
{
	std::string dataS = data;
	nlohmann::json dataJ = nlohmann::json::parse(dataS);
	OnNodeDataLoad(dataJ);
}

#endif