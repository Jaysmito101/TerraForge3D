#include "Generators/CPUNodeEditor/Nodes/TextureCoordinatesNode.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include <iostream>

NodeOutput TextureCoordinatesNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	int id = pin->id;

	if (outputPins[0]->id == id)
		return NodeOutput({input.texX});
	else if (outputPins[1]->id == id)
		return NodeOutput({ input.texY});
	return NodeOutput();
}

void TextureCoordinatesNode::Load(nlohmann::json data)
{
}

nlohmann::json TextureCoordinatesNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::TextureCoordinates;
	return data;
}

void TextureCoordinatesNode::OnRender()
{
	DrawHeader("Texture Coordinates");
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("X");
	outputPins[0]->Render();
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Y");
	outputPins[1]->Render();
}

TextureCoordinatesNode::TextureCoordinatesNode()
{
	headerColor = ImColor(VALUE_NODE_COLOR);
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
}
