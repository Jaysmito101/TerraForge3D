#include "Generators/CPUNodeEditor/Nodes/DuplicateNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>


NodeOutput DuplicateNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	if(inputPins[0]->IsLinked()) return NodeOutput({ inputPins[0]->other->Evaluate(input) });
	return NodeOutput({ 0.0f });
}

void DuplicateNode::Load(nlohmann::json data)
{
}

nlohmann::json DuplicateNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Duplicate;
	return data;
}

void DuplicateNode::OnRender()
{
	DrawHeader("Duplicate");
	inputPins[0]->Render();
	ImGui::Text("Value");
	for (int i = 0; i < 6; i++)
	{
		ImGui::Dummy(ImVec2(150, 20));
		ImGui::SameLine();
		ImGui::Text("Output %d", (i + 1));
		outputPins[i]->Render();
	}
}

DuplicateNode::DuplicateNode()
{
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(OP_NODE_COLOR);
}
