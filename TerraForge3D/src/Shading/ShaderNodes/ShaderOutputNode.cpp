#include "Shading/ShaderNodes/ShaderOutputNode.h"

#include <iostream>

void ShaderOutputNode::OnEvaluate(GLSLFunction* function, GLSLLine* line)
{
	if (inputPins[0]->IsLinked())
	{
		function->AddLine(GLSLLine("", "Not yet Implemented"));
	}
	else
	{
		function->AddLine(GLSLLine("vec3 norm" + STR(id) + " = normalize(Normal);"));
		function->AddLine(GLSLLine("vec3 lightDir" + STR(id) + " = normalize(_LightPosition - FragPos.xyz );"));
		function->AddLine(GLSLLine("float diff" + STR(id) + " = max(dot(norm" + STR(id) + ", lightDir" + STR(id) + "), 0.0f);"));
		function->AddLine(GLSLLine("vec3 diffuse" + STR(id) + " = diff" + STR(id) + " * _LightColor;"));
		function->AddLine(GLSLLine("vec3 result" + STR(id) + " = (vec3(0.2, 0.2, 0.2) + diffuse" + STR(id) + ") * vec3(" + STR(color[0]) + ", " + STR(color[1]) + ", " + STR(color[2]) + ");"));
		line->line = "result" + STR(id);
	}
}

void ShaderOutputNode::Load(nlohmann::json data)
{
	color[0] = data["color.r"];
	color[1] = data["color.g"];
	color[2] = data["color.b"];
	color[3] = data["color.a"];
}

nlohmann::json ShaderOutputNode::Save()
{
	nlohmann::json data;
	data["type"] = "ShaderOutput";
	data["color.r"] = color[0];
	data["color.g"] = color[1];
	data["color.b"] = color[2];
	data["color.a"] = color[3];
	return data;
}

void ShaderOutputNode::OnRender()
{
	DrawHeader("Output");
	inputPins[0]->Render();

	if (!inputPins[0]->IsLinked())
	{
		ImGui::ColorPicker4("Object Color", color);
	}

	else
	{
		ImGui::Dummy(ImVec2(100, 40));
	}
}

ShaderOutputNode::ShaderOutputNode(GLSLHandler* handler)
	:SNENode(handler)
{
	name = "Output";
	color[0] = color[1] = color[2] = color[3] = 0.0f;
	headerColor = ImColor(SHADER_OUTPUT_NODE_COLOR);
	inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float3));
}


ShaderOutputNode::~ShaderOutputNode()
{
}