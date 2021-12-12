#include "NodeEditor.h"

static int uidSeed;

int GenerateUID()
{
    return uidSeed++;
}

void SeUIDSeed(int seed)
{
    uidSeed = seed;
}


nlohmann::json NodeEditorLink::Save()
{
    nlohmann::json data;
    data["type"] = "Link";
    data["from"] = from->id;
    data["to"] = to->id;
    data["id"] = id;
    return data;
}

NodeEditorLink::NodeEditorLink(int id)
    :id(id)
{
}

void NodeEditorPin::Begin()
{
    ImGuiNodeEditor::PinKind pinKind = type == NodeEditorPinType::Input ? ImGuiNodeEditor::PinKind::Input : ImGuiNodeEditor::PinKind::Output;
    ImGuiNodeEditor::BeginPin(id, pinKind);
}

void NodeEditorPin::End()
{
    ImGuiNodeEditor::EndPin();
}

bool NodeEditorPin::ValidateLink(NodeEditorLink* link)
{
    if (link)
        return false;
    return parent->OnLink(this, link);
}

void NodeEditorPin::Link(NodeEditorLink* lnk)
{
    link = lnk;
}

NodeEditorPin::NodeEditorPin(NodeEditorPinType type, int id)
    :type(type), id(id)
{
}

NodeEditorPin::~NodeEditorPin()
{
}

std::vector<NodeEditorPin*> NodeEditorNode::GetPins()
{
    std::vector<NodeEditorPin*> pins;
    pins.insert(
        pins.end(),
        std::make_move_iterator(inputPins.begin()),
        std::make_move_iterator(inputPins.end())
    );
    pins.insert(
        pins.end(),
        std::make_move_iterator(outputPins.begin()),
        std::make_move_iterator(outputPins.end())
    );
    return pins;
}

void NodeEditorNode::Render()
{
    ImGuiNodeEditor::BeginNode(id);
    OnRender();
    ImGuiNodeEditor::EndNode();
}

NodeEditorNode::NodeEditorNode()
{
}

NodeEditorNode::~NodeEditorNode()
{
    for (NodeEditorPin* pin : inputPins)
        delete pin;
    for (NodeEditorPin* pin : outputPins)
        delete pin;
}

nlohmann::json NodeEditor::Save()
{
   nlohmann::json data;
   data["type"] = "NodeEditorData";
   data["serializer"] = "Node Serializer v2.0";
   nlohmann::json tmp;
   for (auto& it : nodes) {
       tmp.push_back(it.second->Save());
   }
   data["nodes"] = tmp;
   tmp = nlohmann::json();
   for (auto& it : links) {
       tmp.push_back(it.second->Save());
   }
   data["links"] = tmp;
   return data;
}

void NodeEditor::Load(nlohmann::json data)
{
    // TODO
}

void NodeEditor::Render()
{
    ImGuiNodeEditor::SetCurrentEditor(context);
    ImGuiNodeEditor::Begin(name.c_str());
    for (auto& it : nodes)
    {
        it.second->Render();
    }

    for (auto& it : links)
    {
        ImGuiNodeEditor::Link(it.second->id, it.second->from->id, it.second->to->id);
    }
    ImGuiNodeEditor::End();
}

NodeEditor::NodeEditor()
{
    context = ImGuiNodeEditor::CreateEditor();
}

NodeEditor::~NodeEditor()
{
    ImGuiNodeEditor::DestroyEditor(context);
}

