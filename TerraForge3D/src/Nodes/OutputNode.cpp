#include "OutputNode.h"
#include "MeshNodeEditor.h"

#include <iostream>

NodeOutput OutputNode::Evaluate(NodeInputParam input)
{
    if (inputPins[0]->IsLinked())
        return NodeOutput({ inputPins[0]->other->parent->Evaluate(input)});
    return NodeOutput({0.0f});
}

void OutputNode::Load(nlohmann::json data)
{
}

nlohmann::json OutputNode::Save()
{
    nlohmann::json data;
    data["type"] = MeshNodeEditor::MeshNodeType::Output;
    return data;
}

void OutputNode::OnRender()
{
    DrawHeader("Output");

    inputPins[0]->Render();
    ImGui::Text("Output");
}

OutputNode::OutputNode()
{
    name = "Output";
    headerColor = ImColor(150, 0, 0);
    inputPins.push_back(new NodeEditorPin());
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
}
