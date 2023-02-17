#include "Generators/CPUNodeEditor/Nodes/OutputNode.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include "Base/DataTexture.h"

#include <iostream>

NodeOutput OutputNode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	if (inputPins[0]->IsLinked()) return NodeOutput({ inputPins[0]->other->Evaluate(input)});
	auto dataTextures = reinterpret_cast<DataTexture*>(input.userData3);
	float v0 = 0.0f, v1 = 0.0f, v2 = 0.0f, v3 = 0.0f;
	dataTextures->GetPixelF(input.texX, input.texY, &v0, &v1, &v2, &v3);
	if (inputPins[1]->IsLinked()) v0 = inputPins[1]->other->Evaluate(input).value;
	if (inputPins[2]->IsLinked()) v1 = inputPins[2]->other->Evaluate(input).value;
	if (inputPins[3]->IsLinked()) v2 = inputPins[3]->other->Evaluate(input).value;
	if (inputPins[4]->IsLinked()) v3 = inputPins[4]->other->Evaluate(input).value;
	dataTextures->SetPixelF(input.texX, input.texY, v0, v1, v2, v3);
	return NodeOutput({value});
}

void OutputNode::Load(nlohmann::json data)
{
	value = data["value"];
}

nlohmann::json OutputNode::Save()
{
	nlohmann::json data;
	data["value"] = value;
	data["type"] = MeshNodeEditor::MeshNodeType::Output;
	return data;
}

void OutputNode::OnRender()
{
	DrawHeader("Output");
	for (int i = 0; i < 5; i++)
	{
		inputPins[i]->Render();
		ImGui::PushItemWidth(100);
		if (inputPins[i]->IsLinked() || i > 0) ImGui::Dummy(ImVec2(100, 40));
		else  UPDATE_HAS_CHHANGED(ImGui::DragFloat(("##" + std::to_string(inputPins[i]->id)).c_str(), &value, 0.01f));
		ImGui::PopItemWidth();
	}
}

OutputNode::OutputNode()
{
	name = "Output";
	value = 0;
	headerColor = ImColor(OUTPUT_NODE_COLOR);
	inputPins.push_back(new NodeEditorPin()); // output
	inputPins.push_back(new NodeEditorPin()); // extra0
	inputPins.push_back(new NodeEditorPin()); // extra1
	inputPins.push_back(new NodeEditorPin()); // extra2
	inputPins.push_back(new NodeEditorPin()); // extra3
	outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
}
