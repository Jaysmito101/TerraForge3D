#include "Shading/ShaderNodes/BakeToSlotNode.h"

#include <iostream>

void BakeToSlotNode::OnEvaluate(GLSLFunction *function, GLSLLine *line)
{
    if(inputPins[0]->IsLinked())
    {
        GLSLLine tmp("", "");
        inputPins[0]->other->Evaluate(GetParams(function, &tmp));
        function->AddLine(GLSLLine("textureBakeSlots[int(" + SDATA(0) + ")] = " + tmp.line + ";"));
	    line->line = tmp.line;
    }
    else
    {
        function->AddLine(GLSLLine("textureBakeSlots[int(" + SDATA(0) + ")] = vec3(0.0f);"));
	    line->line = "vec3(0.0f)";
    }
}

void BakeToSlotNode::Load(nlohmann::json data)
{
	slot = data["slot"];
}

nlohmann::json BakeToSlotNode::Save()
{
	nlohmann::json data;
	data["type"] = "BakeToSlot";
	data["slot"] = slot;
	return data;
}

void BakeToSlotNode::UpdateShaders()
{
	sharedData->d0 = static_cast<float>(slot);
}

void BakeToSlotNode::OnRender()
{
	DrawHeader("Bake to Slot");
	ImGui::PushItemWidth(100);

    ImGui::NewLine();
    inputPins[0]->Render();

	if(ImGui::InputInt("Slot", &slot))
	{
		sharedData->d0 = static_cast<float>(slot);
	}

	outputPins[0]->Render();	

    if(slot == 0)
    {
        ImGui::TextColored(ImVec4(7.0f, 0.0, 0.0f, 1.0f), "Slot 0 is reserved for Height Map!");
    }

    if(slot < 0 || slot > 10)
    {
        slot = 1;
    }

	ImGui::PopItemWidth();
}

BakeToSlotNode::BakeToSlotNode(GLSLHandler *handler)
	:SNENode(handler)
{
	name = "Bake To Slot";
	slot = 1;
	headerColor = ImColor(SHADER_VALUE_NODE_COLOR);
	outputPins.push_back(new SNEPin(NodeEditorPinType::Output, SNEPinType::SNEPinType_Float3));
	inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float3));
}


BakeToSlotNode::~BakeToSlotNode()
{
}