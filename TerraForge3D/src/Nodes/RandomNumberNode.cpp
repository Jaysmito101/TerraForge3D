#include "RandomNumberNode.h"
#include "Base/ImGuiShapes.h"
#include "MeshNodeEditor.h"
#include <iostream>

#include <mutex>

NodeOutput RandomNumberNode::Evaluate(NodeInputParam input, NodeEditorPin* pin)
{
    int s, mx, mn;
    s = mx = mn = 0;
    if (inputPins[0]->IsLinked())
        s = inputPins[0]->other->Evaluate(input).value;
    else
        s = seed;
    if (inputPins[1]->IsLinked())
        mn *= inputPins[1]->other->Evaluate(input).value;
    else
        mn *= min;
    if (inputPins[2]->IsLinked())
        mx *= inputPins[2]->other->Evaluate(input).value;
    else
        mx *= max;
    if(mn > mx)
    {
	int t = mn;
	mn = mx;
	mx = t;
    }
    std::seed_seq seq{seed};
    engine.seed(seq);
    std::uniform_int_distribution<int> dist(mn, mx);
    return NodeOutput({(float)dist(engine)});
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
        ImGui::DragInt(("##" + std::to_string(inputPins[0]->id)).c_str(), &seed, 0.1f);
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
        ImGui::DragInt(("##" + std::to_string(inputPins[1]->id)).c_str(), &min, 0.1f);
        ImGui::PopItemWidth();
    }

    inputPins[2]->Render();
    ImGui::Text("Maximum");
    if (!inputPins[2]->IsLinked())
    {
        ImGui::PushItemWidth(100);
        ImGui::DragInt(("##" + std::to_string(inputPins[2]->id)).c_str(), &max, 0.1f);
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
