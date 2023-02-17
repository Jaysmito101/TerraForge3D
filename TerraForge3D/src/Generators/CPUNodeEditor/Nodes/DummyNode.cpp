#include "Generators/CPUNodeEditor/Nodes/DummyNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

NodeOutput DummyNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	return NodeOutput();
}

bool DummyNode::OnLink(NodeEditorPin *pin, NodeEditorLink *link)
{
	std::cout << "OnLink -> (Pin:" << pin->id << " Link : " << link->to->id << "\n";
	return true;
}

void DummyNode::OnDelete()
{
	std::cout << "OnDelete Node : " << id << "\n";
}

void DummyNode::Load(nlohmann::json data)
{
}

nlohmann::json DummyNode::Save()
{
	nlohmann::json data;
	data["type"] = MeshNodeEditor::MeshNodeType::Dummy;
	return data;
}

void DummyNode::OnRender()
{
	DrawHeader("Dummy Node");
	inputPins[0]->Render();
	ImGui::Text("In");
	ImGui::SameLine();
	ImGui::Text("Out");
	outputPins[0]->Render();
	ImGui::Text("Stats");
	ImGui::Text(("Node ID : " + std::to_string(id)).c_str());
	bool tmp = inputPins[0]->IsLinked();
	ImGui::Text("Inp Pin : ");
	ImGui::SameLine();
	ImGui::Checkbox(("##"+std::to_string(inputPins[0]->id)).c_str(), &tmp);
	ImGui::Text(("ID : " + std::to_string(inputPins[0]->id)).c_str());
	tmp = outputPins[0]->IsLinked();
	ImGui::Text("Out Pin : ");
	ImGui::SameLine();
	ImGui::Checkbox(("##" + std::to_string(inputPins[0]->id)).c_str(), &tmp);
	ImGui::Text(("ID : " + std::to_string(outputPins[0]->id)).c_str());
}

DummyNode::DummyNode()
{
	headerColor = ImColor(DUMMY_NODE_COLOR);
	inputPins.push_back(new NodeEditorPin());
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
}
