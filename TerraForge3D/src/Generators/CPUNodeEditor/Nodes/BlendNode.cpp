#include "Generators/CPUNodeEditor/Nodes/BlendNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>

static const char *items[] = { "Add", "Subtract", "Multiply", "Divide" };

NodeOutput BlendNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	float v1, v2, f;
	v1 = v2 = f = 0;

	if (inputPins[0]->IsLinked())
	{
		v1 = inputPins[0]->other->Evaluate(input).value;
	}

	else
	{
		v1 = value1;
	}

	if (inputPins[1]->IsLinked())
	{
		v2 = inputPins[1]->other->Evaluate(input).value;
	}

	else
	{
		v2 = value2;
	}

	if (inputPins[2]->IsLinked())
	{
		f = inputPins[2]->other->Evaluate(input).value;
	}

	else
	{
		f = factor;
	}

	return NodeOutput({ f * v1 + (1 - f) * v2 });
}

void BlendNode::Load(nlohmann::json data)
{
	value1 = data["value1"];
	value2 = data["value2"];
	factor = data["factor"];
}

nlohmann::json BlendNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Blend;
	data["value1"] = value1;
	data["value2"] = value2;
	data["factor"] = factor;
	return data;
}

void BlendNode::OnRender()
{
	DrawHeader("Blend");
	inputPins[0]->Render();
	ImGui::Text("Value A");

	if (!inputPins[0]->IsLinked())
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[0]->id)).c_str(), &value1, 0.01f));
		ImGui::PopItemWidth();
	}

	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();
	inputPins[1]->Render();
	ImGui::Text("Value B");

	if (!inputPins[1]->IsLinked())
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[1]->id)).c_str(), &value2, 0.01f));
		ImGui::PopItemWidth();
	}

	inputPins[2]->Render();
	ImGui::Text("Factor");

	if (!inputPins[2]->IsLinked())
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[2]->id)).c_str(), &factor, 0.01f, 0, 1));
		ImGui::PopItemWidth();
	}

	ImGui::NewLine();
	ImGui::Text("Blend Mode");
}

BlendNode::BlendNode()
{
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(OP_NODE_COLOR);
	value1 = 0.0f;
	value2 = 1.0f;
	factor = 0.5f;
}
