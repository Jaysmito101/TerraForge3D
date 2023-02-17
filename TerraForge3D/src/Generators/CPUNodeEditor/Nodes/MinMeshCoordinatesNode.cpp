#include "Generators/CPUNodeEditor/Nodes/MinMeshCoordinatesNode.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include <iostream>

NodeOutput MinMeshCoordinatesNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	int id = pin->id;

	if (outputPins[0]->id == id)
		return NodeOutput({input.minX});
	else if (outputPins[1]->id == id)
		return NodeOutput({ input.minY});
	else if (outputPins[2]->id == id)
		return NodeOutput({ input.minZ});
	return NodeOutput();
}

void MinMeshCoordinatesNode::Load(nlohmann::json data)
{
}

nlohmann::json MinMeshCoordinatesNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::MinMeshCoordinates;
	return data;
}

void MinMeshCoordinatesNode::OnRender()
{
	DrawHeader("Minimum Mesh Coordinates");
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Min X");
	outputPins[0]->Render();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Min Y");
	outputPins[1]->Render();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Min Z");
	outputPins[2]->Render();
}

MinMeshCoordinatesNode::MinMeshCoordinatesNode()
{
	headerColor = ImColor(VALUE_NODE_COLOR);
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
}
