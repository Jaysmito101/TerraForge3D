#include "Generators/CPUNodeEditor/Nodes/AbsNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>
#include "Base/ImGuiCurveEditor.h"

NodeOutput AbsNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	float x = 1;
	if (inputPins[0]->IsLinked()) x = inputPins[0]->other->Evaluate(input).value;
	else x = value1;
	if (inputPins[1]->IsLinked()) x *= inputPins[1]->other->Evaluate(input).value;
	else x *= value2;
	return NodeOutput({ abs(x) });
}

void AbsNode::Load(nlohmann::json data)
{
	value1 = data["value1"];
	value2 = data["value2"];
}

nlohmann::json AbsNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Abs;
	data["value1"] = value1;
	data["value2"] = value2;
	return data;
}

void AbsNode::OnRender()
{
	DrawHeader("Absolute Value");
	inputPins[0]->Render();

	if (inputPins[0]->IsLinked())
	{
		ImGui::Text("X");
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

	if (inputPins[1]->IsLinked()) ImGui::Text("k");
	else
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[1]->id)).c_str(), &value2, 0.01f));
		ImGui::PopItemWidth();
	}

	ImGui::Text("Calculates absolute value of kX");
}

AbsNode::AbsNode()
{
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(MATH_NODE_COLOR);
	value1 = (0.0f);
	value2 = (1.0f);
}
