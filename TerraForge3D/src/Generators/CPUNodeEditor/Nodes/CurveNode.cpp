#include "Generators/CPUNodeEditor/Nodes/CurveNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>
#include "Base/ImGuiCurveEditor.h"

// Temporary

template <typename t>
static void ReserveVector(std::vector<t> &vec, int amount)
{
	int size = (int)vec.size();

	for (int i = size; i <= amount; i++)
	{
		vec.push_back(t());
	}
}

static const char *axises[3] = {"X", "Y", "Z"};

NodeOutput CurveNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	float result = 0;
	float axisVal = 0;
	float axisMax = 0;

	switch (axis)
	{
		case 0 :
			axisVal = input.x;
			axisMax = input.maxX;
			break;

		case 1:
			axisVal = input.y;
			axisMax = input.maxY;
			break;

		case 2:
			axisVal = input.z;
			axisMax = input.maxZ;
			break;

		default:
			break;
	}

	if (axisMax == 0)
	{
		axisMax = 1;
	}

	return NodeOutput({ ImGui::CurveValueSmooth((axisVal/axisMax), maxPoints, curve.data()) });
}

void CurveNode::Load(nlohmann::json data)
{
	maxPoints = data["maxPoints"];
	ReserveVector(curve, (maxPoints > data["curveSize"].get<int>() ? maxPoints : data["curveSize"].get<int>()));

	for (nlohmann::json tmp : data["curve"])
	{
		curve[tmp["index"]] = ImVec2(tmp["x"], tmp["y"]);
	}

	axis = data["axis"];
}

nlohmann::json CurveNode::Save()
{
	nlohmann::json data;
	nlohmann::json tmp;
	data["type"] = MeshNodeEditor::MeshNodeType::Curve;
	int i = 0;

	for (ImVec2 point : curve)
	{
		nlohmann::json tmp2;
		tmp2["x"] = point.x;
		tmp2["y"] = point.y;
		tmp2["index"] = i++;
		tmp.push_back(tmp2);
	}

	data["curve"] = tmp;
	data["curveSize"] = curve.size();
	data["maxPoints"] = maxPoints;
	data["axis"] = axis;
	return data;
}

void CurveNode::OnRender()
{
	DrawHeader("Curve Editor");
	ImGui::Dummy(ImVec2(150, 10));
	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();
	ImGui::Text("Max Points");
	ImGui::PushItemWidth(100);

	if (ImGui::DragInt(("##dI" + std::to_string(id)).c_str(), &maxPoints, 1, 10, 256))
	{
		hasChanged = true;
		ReserveVector(curve, maxPoints);
	}

	ImGui::PopItemWidth();
	ImGui::NewLine();

	if (ImGui::Curve(("##" + std::to_string(id)).c_str(), ImVec2(200, 200), maxPoints, curve.data()))
	{
		hasChanged = true;
	}

	ImGui::Text("Current Axis: ");
	ImGui::SameLine();
	ImGui::Text(axises[axis]);

	if (ImGui::Button(("Change Axis##" + std::to_string(id)).c_str()))
	{
		axis = (axis + 1) % 3;
		hasChanged = true;
	}
}

CurveNode::CurveNode()
{
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(OP_NODE_COLOR);
	maxPoints = 10;
	ReserveVector(curve, maxPoints);
	curve[0].x = -1;
	axis = 0;
}
