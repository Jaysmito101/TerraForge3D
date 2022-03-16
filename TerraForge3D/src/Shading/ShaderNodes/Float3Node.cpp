#include "Shading/ShaderNodes/Float3Node.h"

#include <iostream>

void Float3Node::OnEvaluate(GLSLFunction *function, GLSLLine *line)
{
	line->line = "vec3(" + SDATA(0) + ", " + SDATA(1) + ", " + SDATA(2) + ")";
}

void Float3Node::Load(nlohmann::json data)
{
	x = data["x"];
	y = data["y"];
	z = data["z"];
}

nlohmann::json Float3Node::Save()
{
	nlohmann::json data;
	data["type"] = "Float3";
	data["x"] = x;
	data["y"] = y;
	data["z"] = z;
	return data;
}

void Float3Node::UpdateShaders()
{
	sharedData->d0 = x;
	sharedData->d1 = y;
	sharedData->d2 = z;
}

void Float3Node::OnRender()
{
	DrawHeader("Float 3");
	ImGui::PushItemWidth(100);

	if(ImGui::DragFloat("X", &x, 0.01f))
	{
		sharedData->d0 = x;
	}

	ImGui::SameLine();
	outputPins[0]->Render();

	if(ImGui::DragFloat("Y", &y, 0.01f))
	{
		sharedData->d1 = y;
	}

	if(ImGui::DragFloat("Z", &z, 0.01f))
	{
		sharedData->d2 = z;
	}

	ImGui::PopItemWidth();
}

Float3Node::Float3Node(GLSLHandler *handler)
	:SNENode(handler)
{
	name = "Float 3";
	x = y = z = 0.0f;
	headerColor = ImColor(SHADER_VALUE_NODE_COLOR);
	outputPins.push_back(new SNEPin(NodeEditorPinType::Output, SNEPinType::SNEPinType_Float3));
}


Float3Node::~Float3Node()
{
}