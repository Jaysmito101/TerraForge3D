#include "Generators/CPUNodeEditor/Nodes/ClampNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>

NodeOutput ClampNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	float mn, mx, vv;
	mn = 0.0f;
	mx = 1.0f;
	vv = 0.5f;

	if (inputPins[0]->IsLinked())
	{
		vv = inputPins[0]->other->Evaluate(input).value;
	}

	else
	{
		vv = inpt;
	}

	if (inputPins[1]->IsLinked())
	{
		mn = inputPins[1]->other->Evaluate(input).value;
	}

	else
	{
		mn = minV;
	}

	if (inputPins[2]->IsLinked())
	{
		mx = inputPins[2]->other->Evaluate(input).value;
	}

	else
	{
		mx = maxV;
	}

	return NodeOutput({ vv > mx ? mx : ( vv < mn ? mn : vv ) });
}

void ClampNode::Load(nlohmann::json data)
{
	minV = data["minV"];
	maxV = data["maxV"];
	inpt = data["inpt"];
}

nlohmann::json ClampNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Clamp;
	data["minV"] = minV;
	data["maxV"] = maxV;
	data["inpt"] = inpt;
	return data;
}

void ClampNode::OnRender()
{
	DrawHeader("Clamp");
	inputPins[0]->Render();
	ImGui::Text("Input");

	if (!inputPins[0]->IsLinked())
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[0]->id)).c_str(), &inpt, 0.01f));
		ImGui::PopItemWidth();
	}

	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();
	inputPins[1]->Render();
	ImGui::Text("Minimum");

	if (!inputPins[1]->IsLinked())
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[1]->id)).c_str(), &minV, 0.01f));
		ImGui::PopItemWidth();
	}

	inputPins[2]->Render();
	ImGui::Text("Maximum");

	if (!inputPins[2]->IsLinked())
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[2]->id)).c_str(), &maxV, 0.01f));
		ImGui::PopItemWidth();
	}

	ImGui::NewLine();
}

ClampNode::ClampNode()
{
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(OP_NODE_COLOR);
	minV = 0.0f;
	maxV = 1.0f;
	inpt = 0.5f;
}
