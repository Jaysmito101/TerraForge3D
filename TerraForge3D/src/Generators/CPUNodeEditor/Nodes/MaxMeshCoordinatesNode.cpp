#include "Generators/CPUNodeEditor/Nodes/MaxMeshCoordinatesNode.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include <iostream>

NodeOutput MaxMeshCoordinatesNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	int id = pin->id;

	if (outputPins[0]->id == id)
		return NodeOutput({input.maxX});
	else if (outputPins[1]->id == id)
		return NodeOutput({ input.maxY});
	else if (outputPins[2]->id == id)
		return NodeOutput({ input.maxZ});
	return NodeOutput();
}

void MaxMeshCoordinatesNode::Load(nlohmann::json data)
{
}

nlohmann::json MaxMeshCoordinatesNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::MaxMeshCoordinates;
	return data;
}

void MaxMeshCoordinatesNode::OnRender()
{
	DrawHeader("Maximum Mesh Coordinates");
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Max X");
	outputPins[0]->Render();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Max Y");
	outputPins[1]->Render();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Max Z");
	outputPins[2]->Render();
}

MaxMeshCoordinatesNode::MaxMeshCoordinatesNode()
{
	headerColor = ImColor(VALUE_NODE_COLOR);
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
}
