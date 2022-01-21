#include "MinValNode.h"
#include "Base/ImGuiShapes.h"
#include "MeshNodeEditor.h"
#include <iostream>

#include <mutex>
#include "Base/ImGuiCurveEditor.h"

NodeOutput MinValNode::Evaluate(NodeInputParam input, NodeEditorPin* pin)
{
    float x = inputf;
    if (inputPins[0]->IsLinked())
        x = inputPins[0]->other->Evaluate(input).value;
    float y = thresholdf;
    if (inputPins[1]->IsLinked())
        y = inputPins[1]->other->Evaluate(input).value;
    if(x > thresholdf)
        return NodeOutput({ outputr });
    else
        return NodeOutput({ outputf });
}

void MinValNode::Load(nlohmann::json data)
{
    inputf = data["inputf"];
    outputf = data["outputf"];
    thresholdf = data["thresholdf"];
}

nlohmann::json MinValNode::Save()
{
    nlohmann::json data;
    data["type"] = MeshNodeEditor::MeshNodeType::MinVal;
    data["inputf"] = inputf;
    data["outputf"] = outputf;
    data["thresholdf"] = thresholdf;
    return data;
}

void MinValNode::OnRender()
{
    DrawHeader("Min Val");

    inputPins[0]->Render();
    if (inputPins[0]->IsLinked())
        ImGui::Text("Input");
    else
    {
        ImGui::PushItemWidth(100);
        ImGui::DragFloat(("##" + std::to_string(inputPins[0]->id)).c_str(), &inputf, 0.01f);
        ImGui::PopItemWidth();
    }

    ImGui::SameLine();

    ImGui::Text("Output");
    outputPins[0]->Render();

    inputPins[1]->Render();
    if (inputPins[1]->IsLinked())
        ImGui::Text("Threshold");
    else
    {
        ImGui::PushItemWidth(100);
        ImGui::DragFloat(("##" + std::to_string(inputPins[1]->id)).c_str(), &thresholdf, 0.01f);
        ImGui::PopItemWidth();
    }

    ImGui::PushItemWidth(100);
    ImGui::DragFloat(("##" + std::to_string(inputPins[2]->id)).c_str(), &outputf, 0.01f);
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(100);
    ImGui::DragFloat(("##" + std::to_string(inputPins[3]->id)).c_str(), &outputr, 0.01f);
    ImGui::PopItemWidth();
}

MinValNode::MinValNode()
{
    inputPins.push_back(new NodeEditorPin());
    inputPins.push_back(new NodeEditorPin());
    inputPins.push_back(new NodeEditorPin()); // FOR FUTURE
    inputPins.push_back(new NodeEditorPin()); // FOR FUTURE
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
    headerColor = ImColor(OP_NODE_COLOR);
    outputf = inputf = outputr = thresholdf = 0.0f;
    thresholdf = 0.5f;
}
