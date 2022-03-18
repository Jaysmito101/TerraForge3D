#include "Shading/ShaderNodes/ShaderOutputNode.h"

#include <iostream>

void ShaderOutputNode::OnEvaluate(GLSLFunction *function, GLSLLine *line)
{
	if (inputPins[0]->IsLinked())
	{
		GLSLLine ln("");
		inputPins[0]->other->Evaluate(GetParams(function, &ln));
		line->line = ln.line;
	}

	else
	{
		function->AddLine(GLSLLine("vec3 " + VAR("norm") + " = normalize(Normal);"));
		function->AddLine(GLSLLine("vec3 " + VAR("lightDir") + " = normalize(_LightPosition - FragPos.xyz );"));
		function->AddLine(GLSLLine("float " + VAR("diff") + " = max(dot(" + VAR("norm") + ", " + VAR("lightDir") + "), 0.0f);"));
		function->AddLine(GLSLLine("vec3 " + VAR("diffuse") + " = " + VAR("diff") + " * _LightColor;"));
		function->AddLine(GLSLLine("vec3 " + VAR("result") + " = (vec3(0.2, 0.2, 0.2) + " + VAR("diffuse") + ")" + \
		                           " * vec3(" + SDATA(0) + ", " + SDATA(1) + ", " + SDATA(2) + ");"));
		line->line = VAR("result");
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

void ShaderOutputNode::UpdateShaders()
{
	sharedData->d0 = color[0];
	sharedData->d1 = color[1];
	sharedData->d2 = color[2];
}

void ShaderOutputNode::OnRender()
{
	DrawHeader("Output");
	inputPins[0]->Render();

	if (!inputPins[0]->IsLinked())
	{
		if(ImGui::ColorPicker4("Object Color", color))
		{
			sharedData->d0 = color[0];
			sharedData->d1 = color[1];
			sharedData->d2 = color[2];
		}
	}

	else
	{
		ImGui::Dummy(ImVec2(100, 40));
	}
}

ShaderOutputNode::ShaderOutputNode(GLSLHandler *handler)
	:SNENode(handler)
{
	name = "Output";
	color[0] = color[1] = color[2] = color[3] = 1.0f;
	headerColor = ImColor(SHADER_OUTPUT_NODE_COLOR);
	inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float3));
}


ShaderOutputNode::~ShaderOutputNode()
{
}