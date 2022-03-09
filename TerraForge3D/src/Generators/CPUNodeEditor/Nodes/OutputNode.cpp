#include "Generators/CPUNodeEditor/Nodes/OutputNode.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include <iostream>

NodeOutput OutputNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	if (inputPins[0]->IsLinked())
		return NodeOutput({ inputPins[0]->other->Evaluate(input)});
	return NodeOutput({value});
}

void OutputNode::Load(nlohmann::json data)
{
	value = data["value"];
}

nlohmann::json OutputNode::Save()
{
	nlohmann::json data;
	data["value"] = value;
	data["type"] = MeshNodeEditor::MeshNodeType::Output;
	return data;
}

void OutputNode::OnRender()
{
	DrawHeader("Output");
	inputPins[0]->Render();

	if (!inputPins[0]->IsLinked())
	{
		ImGui::PushItemWidth(100);
		ImGui::DragFloat(("##" + std::to_string(inputPins[0]->id)).c_str(), &value, 0.01f);
		ImGui::PopItemWidth();
	}

	else
	{
		ImGui::Dummy(ImVec2(100, 40));
	}
}

OutputNode::OutputNode()
{
	name = "Output";
	value = 0;
	headerColor = ImColor(OUTPUT_NODE_COLOR);
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
}
