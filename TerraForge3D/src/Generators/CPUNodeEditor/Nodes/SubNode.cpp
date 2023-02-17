#include "Generators/CPUNodeEditor/Nodes/SubNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>


NodeOutput SubNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	float diff = 0;

	if (inputPins[0]->IsLinked())
	{
		diff += inputPins[0]->other->Evaluate(input).value;
	}

	else
	{
		diff += value1;
	}

	if (inputPins[1]->IsLinked())
	{
		diff -= inputPins[1]->other->Evaluate(input).value;
	}

	else
	{
		diff -= value2;
	}

	return NodeOutput({ diff });
}

void SubNode::Load(nlohmann::json data)
{
	value1 = data["value1"];
	value2 = data["value2"];
}

nlohmann::json SubNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Sub;
	data["value1"] = value1;
	data["value2"] = value2;
	return data;
}

void SubNode::OnRender()
{
	DrawHeader("Subtract");
	inputPins[0]->Render();

	if (inputPins[0]->IsLinked())
	{
		ImGui::Text("Input 1");
	}

	else
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[0]->id)).c_str(), &value1, 0.01f));
		ImGui::PopItemWidth();
	}

	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();
	inputPins[1]->Render();

	if(inputPins[1]->IsLinked())
	{
		ImGui::Text("Input 2");
	}

	else
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[1]->id)).c_str(), &value2, 0.01f));
		ImGui::PopItemWidth();
	}
}

SubNode::SubNode()
{
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(MATH_NODE_COLOR);
	value1 = (0.0f);
	value2 = (0.0f);
}
