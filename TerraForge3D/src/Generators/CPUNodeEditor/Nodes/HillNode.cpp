#include "Generators/CPUNodeEditor/Nodes/HillNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>
#include "Base/ImGuiCurveEditor.h"

#define SQUARE(x) (x) * (x)
#define MIN(x, y) x > y ? x : y


NodeOutput HillNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	float xC, yC, hC;

	if (inputPins[0]->IsLinked())
	{
		xC = inputPins[0]->other->Evaluate(input).value;
	}

	else
	{
		xC = pos[0];
	}

	if (inputPins[1]->IsLinked())
	{
		yC = inputPins[1]->other->Evaluate(input).value;
	}

	else
	{
		yC = pos[1];
	}

	if (inputPins[2]->IsLinked())
	{
		hC = inputPins[2]->other->Evaluate(input).value;
	}

	else
	{
		hC = height;
	}

	float xN = input.x / input.maxX;
	float yN = input.z / input.maxZ; // TEMP
	xN = xN * 2 - 1;
	yN = yN * 2 - 1;
	float h = 1 - (SQUARE((yN - yC) / radius) + SQUARE((xN - xC) / radius));
	h *= powf(2.71828f, - p * (SQUARE(xN - xC) + SQUARE(yN - yC)));
	return NodeOutput({ h * height });
}

void HillNode::Load(nlohmann::json data)
{
	pos[0] = data["posX"];
	pos[1] = data["posY"];
	pos[2] = data["posZ"];
	height = data["height"];
	radius = data["radius"];
	p = data["p"];
}

nlohmann::json HillNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Hill;
	data["posX"] = pos[0];
	data["posY"] = pos[1];
	data["posZ"] = pos[2];
	data["height"] = height;
	data["radius"] = radius;
	data["p"] = p;
	return data;
}

void HillNode::OnRender()
{
	DrawHeader("Hill");
	ImGui::Dummy(ImVec2(150, 20));
	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();
	ImGui::PushItemWidth(150);
	UPDATE_HAS_CHHANGED(ImGui::DragFloat2(MAKE_IMGUI_LABEL(inputPins[0]->id, "Position"), pos, 0.01f));
	UPDATE_HAS_CHHANGED(ImGui::DragFloat(MAKE_IMGUI_LABEL(inputPins[1]->id, "Height"), &height, 0.01f));
	UPDATE_HAS_CHHANGED(ImGui::DragFloat(MAKE_IMGUI_LABEL(inputPins[2]->id, "Radius"), &radius, 0.01f, 0.00001f));
	UPDATE_HAS_CHHANGED(ImGui::DragFloat(MAKE_IMGUI_LABEL(inputPins[3]->id, "Plane Factor"), &p, 0.01f, 0.00001f));
	ImGui::PopItemWidth();
}

HillNode::HillNode()
{
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(MATH_NODE_COLOR);
	pos[0] = pos[1] = pos[2] = 0;
	height = 1.0f;
	radius = 1.0f;
	p = 1.0f;
}
