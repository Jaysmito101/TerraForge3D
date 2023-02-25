#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include <iostream>
#include <mutex>

#include "Generators/CPUNodeEditor/Nodes/RandomNumberNode.h"


static uint32_t fastRandomNumber(int seed)
{
	seed = (214013 * seed + 2531011);
	return (seed >> 16) & 0x7FFF;
}

NodeOutput RandomNumberNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	int s, mx, mn;
	s = mx = mn = 0;

	if (inputPins[0]->IsLinked()) s = (int)inputPins[0]->other->Evaluate(input).value;
	else s = seed;

	if (inputPins[1]->IsLinked()) mn = (int)(mn * inputPins[1]->other->Evaluate(input).value);
	else mn *= min;

	if (inputPins[2]->IsLinked()) mx = (int)(mn * inputPins[2]->other->Evaluate(input).value);
	else mx *= max;

	if(mn > mx) std::swap(mn, mx);

	//std::seed_seq seq{seed};
	//engine.seed(seq);
	//std::uniform_int_distribution<int> dist(mn, mx);
	//return NodeOutput({(float)dist(engine)});
	return NodeOutput({(float)fastRandomNumber(s) / 32767.0f});
}

void RandomNumberNode::Load(nlohmann::json data)
{
	seed = data["seed"];
	min = data["min"];
	max = data["max"];
}

nlohmann::json RandomNumberNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::RandomNumber;
	data["seed"] = seed;
	data["min"] = min;
	data["max"] = max;
	return data;
}

void RandomNumberNode::OnRender()
{
	DrawHeader("Random Number");
	inputPins[0]->Render();
	ImGui::Text("Seed");

	if (!inputPins[0]->IsLinked())
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragInt(("##" + std::to_string(inputPins[0]->id)).c_str(), &seed, 0.1f));
		ImGui::PopItemWidth();
	}

	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();
	inputPins[1]->Render();
	ImGui::Text("Minimum");

	if (!inputPins[1]->IsLinked())
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragInt(("##" + std::to_string(inputPins[1]->id)).c_str(), &min, 0.1f));
		ImGui::PopItemWidth();
	}

	inputPins[2]->Render();
	ImGui::Text("Maximum");

	if (!inputPins[2]->IsLinked())
	{
		ImGui::PushItemWidth(100);
		UPDATE_HAS_CHHANGED(ImGui::DragInt(("##" + std::to_string(inputPins[2]->id)).c_str(), &max, 0.1f));
		ImGui::PopItemWidth();
	}
}

RandomNumberNode::RandomNumberNode()
{
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
	headerColor = ImColor(NOISE_NODE_COLOR);
	seed = 42;
	min = 0;
	max = 10;
}
