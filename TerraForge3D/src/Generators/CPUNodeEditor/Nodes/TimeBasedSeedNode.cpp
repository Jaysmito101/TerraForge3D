#include "Generators/CPUNodeEditor/Nodes/TimeBasedSeedNode.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include <iostream>

NodeOutput TimeBasedSeedNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	return NodeOutput({(float)val});
}

void TimeBasedSeedNode::Load(nlohmann::json data)
{
	val = data["val"];
}

nlohmann::json TimeBasedSeedNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::TimeBasedSeed;
	data["val"] = val;
	return data;
}

void TimeBasedSeedNode::OnRender()
{
	DrawHeader("Time Based Seed");
	ImGui::Dummy(ImVec2(200, 10));
	ImGui::SameLine();
	ImGui::Text("Time");
	outputPins[0]->Render();
	ImGui::NewLine();
	ImGui::Text("Current Seed : ");
	ImGui::SameLine();
	ImGui::PushItemWidth(200);
	UPDATE_HAS_CHHANGED(ImGui::InputInt(MAKE_IMGUI_ID(id), &val, 1));
	ImGui::PopItemWidth();
	ImGui::NewLine();

	if (ImGui::Button("Regenerate Seed"))
	{
		val = (int)time(NULL);
		hasChanged = true;
	}
}

TimeBasedSeedNode::TimeBasedSeedNode()
{
	headerColor = ImColor(VALUE_NODE_COLOR);
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	val = 42;
}
