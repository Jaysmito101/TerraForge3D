#include "Generators/CPUNodeEditor/Nodes/PixelateNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>
#include "Base/ImGuiCurveEditor.h"

NodeOutput PixelateNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	float x = 1;

	if (input.maxZ == 0)
	{
		input.maxZ = 1;
	}

	float currX = input.x / input.maxX;
	float currY = input.y / input.maxY;
	float currZ = input.z / input.maxZ;
	float boxX = floorf(currX / pixelSize);
	float boxY = floorf(currY / pixelSize);
	float boxZ = floorf(currZ / pixelSize);
	input.x = boxX * pixelSize * input.maxX;
	input.y = boxY * pixelSize * input.maxY;
	input.z = boxZ * pixelSize * input.maxZ;

	if(inputPins[0]->IsLinked())
		return NodeOutput({ inputPins[0]->other->Evaluate(input).value });
	return NodeOutput({0.0f});
}

void PixelateNode::Load(nlohmann::json data)
{
	pixelSize = data["pixelSize"];
}

nlohmann::json PixelateNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Pixelate;
	data["pixelSize"] = pixelSize;
	return data;
}

void PixelateNode::OnRender()
{
	DrawHeader("Pixelate Node");
	inputPins[0]->Render();
	ImGui::Text("In");
	inputPins[1]->Render();

	if (inputPins[1]->IsLinked()) ImGui::Text("Pixel Size");
	else
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[0]->id)).c_str(), &pixelSize, 0.01f, 0.001f, 1));
		ImGui::PopItemWidth();
	}

	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();
	ImGui::Text("Pixelates the input");
}

PixelateNode::PixelateNode()
{
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(OP_NODE_COLOR);
	pixelSize = 0.05f;
}
