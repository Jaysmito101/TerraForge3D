#include "DuplicateNode.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>

#include <mutex>


NodeOutput DuplicateNode::Evaluate(NodeInputParam input, NodeEditorPin* pin)
{
    if(inputPins[0]->IsLinked())
        return NodeOutput({ inputPins[0]->other->Evaluate(input) });
    return NodeOutput({ 0.0f });
}

void DuplicateNode::Load(nlohmann::json data)
{
}

nlohmann::json DuplicateNode::Save()
{
    nlohmann::json data;
    data["type"] = MeshNodeEditor::MeshNodeType::Duplicate;
    return data;
}

void DuplicateNode::OnRender()
{
    DrawHeader("Duplicate");

    inputPins[0]->Render();
    ImGui::Text("Value");

    
    ImGui::Dummy(ImVec2(150, 20));
    ImGui::SameLine();
    ImGui::Text("Out 1");
    outputPins[0]->Render();

    ImGui::Dummy(ImVec2(150, 20));
    ImGui::SameLine();
    ImGui::Text("Out 2");
    outputPins[1]->Render();

    ImGui::Dummy(ImVec2(150, 20));
    ImGui::SameLine();
    ImGui::Text("Out 3");
    outputPins[2]->Render();

    ImGui::Dummy(ImVec2(150, 20));
    ImGui::SameLine();
    ImGui::Text("Out 4");
    outputPins[3]->Render();    
}

DuplicateNode::DuplicateNode()
{
    inputPins.push_back(new NodeEditorPin());
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
    headerColor = ImColor(OP_NODE_COLOR);
}
