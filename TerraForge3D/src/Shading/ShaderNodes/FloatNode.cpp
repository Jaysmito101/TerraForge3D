#include "Shading/ShaderNodes/FloatNode.h"

#include <iostream>

void FloatNode::OnEvaluate(GLSLFunction *function, GLSLLine *line)
{
	line->line = SDATA(0);
}

void FloatNode::Load(nlohmann::json data)
{
	x = data["x"];
}

nlohmann::json FloatNode::Save()
{
	nlohmann::json data;
	data["type"] = "Float";
	data["x"] = x;
	return data;
}

void FloatNode::UpdateShaders()
{
	sharedData->d0 = x;
}

void FloatNode::OnRender()
{
	DrawHeader("Float Value");
	ImGui::PushItemWidth(100);

	if(ImGui::DragFloat("X", &x, 0.01f))
	{
		sharedData->d0 = x;
	}

	ImGui::SameLine();
	outputPins[0]->Render();

	ImGui::PopItemWidth();
}

FloatNode::FloatNode(GLSLHandler *handler)
	:SNENode(handler)
{
	name = "Float Value";
	x = y = z = 0.0f;
	headerColor = ImColor(SHADER_VALUE_NODE_COLOR);
	outputPins.push_back(new SNEPin(NodeEditorPinType::Output, SNEPinType::SNEPinType_Float));
}


FloatNode::~FloatNode()
{
}