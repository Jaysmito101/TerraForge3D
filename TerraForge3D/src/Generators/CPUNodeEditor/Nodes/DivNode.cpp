#include "Generators/CPUNodeEditor/Nodes/DivNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>


NodeOutput DivNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	float quo = 0;

	if (inputPins[0]->IsLinked())
	{
		quo = inputPins[0]->other->Evaluate(input).value;
	}

	else
	{
		quo = value1;
	}

	float tmp = 1;

	if (inputPins[1]->IsLinked())
	{
		tmp = inputPins[1]->other->Evaluate(input).value;
	}

	else
	{
		tmp = value2;
	}

	if (tmp == 0)
	{
		tmp = 1;
	}

	quo = quo / tmp;
	return NodeOutput({ quo });
}

void DivNode::Load(nlohmann::json data)
{
	value1 = data["value1"];
	value2 = data["value2"];
}

nlohmann::json DivNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Div;
	data["value1"] = value1;
	data["value2"] = value2;
	return data;
}

void DivNode::OnRender()
{
	DrawHeader("Divide");
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

DivNode::DivNode()
{
	headerColor = ImColor(MATH_NODE_COLOR);
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	value1 = (1.0f);
	value2 = (1.0f);
}
