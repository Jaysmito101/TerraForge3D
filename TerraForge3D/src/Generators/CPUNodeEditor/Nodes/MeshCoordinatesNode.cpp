#include "Generators/CPUNodeEditor/Nodes/MeshCoordinatesNode.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include <iostream>

NodeOutput MeshCoordinatesNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	int id = pin->id;

	if (outputPins[0]->id == id)
		return NodeOutput({input.x});
	else if (outputPins[1]->id == id)
		return NodeOutput({ input.y });
	else if (outputPins[2]->id == id)
		return NodeOutput({ input.z });
	return NodeOutput();
}

void MeshCoordinatesNode::Load(nlohmann::json data)
{
}

nlohmann::json MeshCoordinatesNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::MeshCoordinates;
	return data;
}

void MeshCoordinatesNode::OnRender()
{
	DrawHeader("Mesh Coordinates");
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("X");
	outputPins[0]->Render();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Y");
	outputPins[1]->Render();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Z");
	outputPins[2]->Render();
}

MeshCoordinatesNode::MeshCoordinatesNode()
{
	headerColor = ImColor(VALUE_NODE_COLOR);
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
}
