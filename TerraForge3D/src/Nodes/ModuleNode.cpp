#include "ModuleNode.h"
#include "Base/ImGuiShapes.h"
#include "MeshNodeEditor.h"
#include <iostream>

#include <mutex>
#include "Base/ImGuiCurveEditor.h"

NodeOutput ModuleNode::Evaluate(NodeInputParam input, NodeEditorPin* pin)
{
    float x = 1;
    return NodeOutput({ abs(x) });
}

void ModuleNode::Load(nlohmann::json data)
{
    id = data["mid"];
}

nlohmann::json ModuleNode::Save()
{
    nlohmann::json data;
    data["type"] = MeshNodeEditor::MeshNodeType::Abs;
    data["mid"] = id;
    return data;
}

void ModuleNode::OnRender()
{
    if(mod)
        DrawHeader(mod->name);

    ImGui::Dummy(ImVec2(150, 10));   
    ImGui::SameLine();
    ImGui::Text("Out");
    outputPins[0]->Render();

    if (mod)
        mod->Render();
    else
        ImGui::Text("Failed to load module!");
}

ModuleNode::ModuleNode(std::string id, NodeModule* mod)
    :id(id), mod(mod)
{
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
    headerColor = ImColor(OP_NODE_COLOR);
}
