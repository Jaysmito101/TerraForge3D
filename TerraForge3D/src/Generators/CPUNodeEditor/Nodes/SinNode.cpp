#include "SinNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>


NodeOutput SinNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	float x = 1;

	if (inputPins[0]->IsLinked())
	{
		x = inputPins[0]->other->Evaluate(input).value;
	}

	else
	{
		x = value1;
	}

	if (inputPins[1]->IsLinked())
	{
		x *= inputPins[1]->other->Evaluate(input).value;
	}

	else
	{
		x *= value2;
	}

	return NodeOutput({ sin(x) });
}

void SinNode::Load(nlohmann::json data)
{
	value1 = data["value1"];
	value2 = data["value2"];
}

nlohmann::json SinNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Sin;
	data["value1"] = value1;
	data["value2"] = value2;
	return data;
}

void SinNode::OnRender()
{
	DrawHeader("Sin");
	inputPins[0]->Render();

	if (inputPins[0]->IsLinked())
	{
		ImGui::Text("X");
	}

	else
	{
		ImGui::PushItemWidth(100);
		ImGui::DragFloat(("##" + std::to_string(inputPins[0]->id)).c_str(), &value1, 0.01f);
		ImGui::PopItemWidth();
	}

	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();
	inputPins[1]->Render();

	if (inputPins[1]->IsLinked())
	{
		ImGui::Text("k");
	}

	else
	{
		ImGui::PushItemWidth(100);
		ImGui::DragFloat(("##" + std::to_string(inputPins[1]->id)).c_str(), &value2, 0.01f);
		ImGui::PopItemWidth();
	}

	ImGui::Text("Calculates sin(kX)");
}

SinNode::SinNode()
{
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(MATH_NODE_COLOR);
	value1 = (0.0f);
	value2 = (1.0f);
}
