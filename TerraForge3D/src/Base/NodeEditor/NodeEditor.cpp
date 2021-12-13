#include "NodeEditor.h"
#include "Base/ImGuiShapes.h"

static int uidSeed = 1;

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
    nlohmann::json tmp;
    data["type"] = "Link";
    data["from"] = from->id;
    data["to"] = to->id;
    data["id"] = id;
    tmp["r"] = color.x;
    tmp["g"] = color.y;
    tmp["b"] = color.z;
    tmp["a"] = color.w;
    data["color"] = tmp;
    tmp = nlohmann::json();
    data["thickness"] = thickness;
    return data;
}

NodeEditorLink::NodeEditorLink(int id)
    :id(id)
{
    _id = id;
}

nlohmann::json NodeEditorPin::Save()
{
    return nlohmann::json();
}

void NodeEditorPin::Load(nlohmann::json data)
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

bool NodeEditorPin::ValidateLink(NodeEditorLink* lnk)
{
    if (link)
        return false;
    if (lnk->from->type == lnk->to->type)
        return false;

    return parent->OnLink(this, lnk);
}

void NodeEditorPin::Link(NodeEditorLink* lnk)
{
    link = lnk;
    if (link->from->id == id)
        other = link->to;
    else
        other = link->from;
}

bool NodeEditorPin::IsLinked()
{
    return link;
}

NodeEditorPin::NodeEditorPin(NodeEditorPinType type, int id)
    :type(type), id(id), link(nullptr)
{
    _id = id;
}

void NodeEditorPin::Render()
{
    if (type == NodeEditorPinType::Output)
        ImGui::SameLine();
    Begin();
    if(IsLinked())
        ImGui::DrawFilledCircle(10, color);
    else
        ImGui::DrawCircle(10, color);
    ImGui::Dummy(ImVec2(20, 20));
    End();
    if(type== NodeEditorPinType::Input)
        ImGui::SameLine();
}

NodeEditorPin::~NodeEditorPin()
{
}

void NodeEditorPin::Unlink()
{
    link = nullptr;
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

bool NodeEditorNode::OnLink(NodeEditorPin* pin, NodeEditorLink* link)
{
    return true;
}

void NodeEditorNode::OnDelete()
{
}

void NodeEditorNode::Render()
{
    ImGuiNodeEditor::BeginNode(id);
    OnRender();
    ImGuiNodeEditor::EndNode();
}

NodeEditorNode::NodeEditorNode(int id)
    :id(id)
{
    _id = id;
}

NodeEditorNode::~NodeEditorNode()
{
}

void NodeEditorNode::DrawHeader(std::string text)
{
    ImVec2 pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(pos.x - ImGuiNodeEditor::GetStyle().NodePadding.x, pos.y - ImGuiNodeEditor::GetStyle().NodePadding.y));
    ImVec2 start = ImGuiNodeEditor::GetNodeSize(_id);
    
    ImGui::DrawFilledRect(ImVec2(start.x, 40), headerColor, 13);
    ImGui::SetCursorPos(ImVec2(pos.x + ImGuiNodeEditor::GetStyle().NodePadding.x, pos.y + ImGuiNodeEditor::GetStyle().NodePadding.x));
    ImGui::Text(text.c_str());
    ImGui::NewLine();
}

void NodeEditorNode::Setup()
{
    for (NodeEditorPin* pin : inputPins)
        pin->parent = this;
    for (NodeEditorPin* pin : outputPins)
        pin->parent = this;
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
    bool windowFocused = ImGui::IsWindowFocused();        
    ImGuiNodeEditor::SetCurrentEditor(context);
    ImGuiNodeEditor::Begin(name.c_str());
    ImGuiNodeEditor::EnableShortcuts(windowFocused);
    for (auto& it : nodes)
    {
        it.second->Render();
    }

    for (auto& it : links)
    {
        ImGuiNodeEditor::Link(it.second->id, it.second->from->id, it.second->to->id, it.second->color, it.second->thickness);
    }
    if (ImGuiNodeEditor::BeginCreate())
    {
        ImGuiNodeEditor::PinId iPid, oPid;
        if (ImGuiNodeEditor::QueryNewLink(&iPid, &oPid))
        {
            if (iPid && oPid) {
                if (ImGuiNodeEditor::AcceptNewItem())
                {
                    NodeEditorLink* link = new NodeEditorLink();
                    link->from = pins[iPid.Get()];
                    link->to = pins[oPid.Get()];
                    bool lnkFrm = link->from->ValidateLink(link);
                    bool lnkTo = link->to->ValidateLink(link);
                    if (lnkFrm && lnkTo)
                    {
                        link->from->Link(link);
                        link->to->Link(link);
                        links[link->_id.Get()] = link;
                        ImGuiNodeEditor::Link(link->id, link->from->id, link->to->id);
                    }
                }
            }
            else if(iPid) {
                NodeEditorLink* link = links[iPid.Get()];
                link->from->link = nullptr;
                link->to->link = nullptr;
                links.erase(links.find(iPid.Get()));
                delete link;
            }
        }

        ImGuiNodeEditor::PinId nPid;
        if (makeNodeFunc && ImGuiNodeEditor::QueryNewNode(&nPid))
        {
            if (ImGuiNodeEditor::AcceptNewItem())
            {
                NodeEditorPin* iPin = pins[nPid.Get()];
                NodeEditorNode* node = makeNodeFunc(iPin);
                if (node)
                {
                    AddNode(node);
                    if (node->inputPins.size() > 0)
                    {
                        NodeEditorLink* link = new NodeEditorLink();
                        link->from = iPin;
                        link->to = node->inputPins[0];
                        bool lnkFrm = link->from->ValidateLink(link);
                        bool lnkTo = link->to->ValidateLink(link);
                        if (lnkFrm && lnkTo)
                        {
                            link->from->Link(link);
                            link->to->Link(link);
                            links[link->_id.Get()] = link;
                            ImGuiNodeEditor::Link(link->id, link->from->id, link->to->id);
                        }
                    }                    
                }
            }
        }
    }
    ImGuiNodeEditor::EndCreate();

    if (ImGuiNodeEditor::BeginDelete())
    {
        ImGuiNodeEditor::NodeId nId;
        if (ImGuiNodeEditor::QueryDeletedNode(&nId))
        {
            if (nId && ImGuiNodeEditor::AcceptDeletedItem())
            {
                NodeEditorNode* node = nodes[nId.Get()];
                DeleteNode(node);
            }
        }
        else
        {
            ImGuiNodeEditor::LinkId lId;
            if (ImGuiNodeEditor::QueryDeletedLink(&lId))
            {
                if (lId && ImGuiNodeEditor::AcceptDeletedItem())
                {
                    NodeEditorLink* link = links[lId.Get()];
                    link->from->link = nullptr;
                    link->to->link = nullptr;
                    links.erase(links.find(lId.Get()));
                    delete link;
                }
            }
        }
    }
    ImGuiNodeEditor::EndDelete();

    if (updateFunc)
        updateFunc();
    ImGuiNodeEditor::End();
    ImGuiNodeEditor::SetCurrentEditor(nullptr);
}

void NodeEditor::AddNode(NodeEditorNode* node)
{
    nodes[node->_id.Get()] = node;
    for (auto& it : node->GetPins()) 
    {
        pins[it->_id.Get()] = it;
    }
    node->Setup();
}

NodeEditor::NodeEditor(NodeEditorConfig aconfig)
{
    config = aconfig;
    ImGuiNodeEditor::Config iconfig;
    iconfig.SettingsFile = config.saveFile.c_str();
    context = ImGuiNodeEditor::CreateEditor(&iconfig);
}

void NodeEditor::DeleteNode(NodeEditorNode* node)
{
    if (nodes.find(node->_id.Get()) != nodes.end()) 
    {
        node->OnDelete();
        std::vector<NodeEditorPin*> mPins = node->GetPins();
        for (NodeEditorPin* pin : mPins) {
            if (pin->IsLinked()) {
                links.erase(links.find(pin->link->_id.Get()));
                pin->other->Unlink();
                delete pin->link;
            }
            pins.erase(pins.find(pin->_id.Get()));
            delete pin;
        }
        nodes.erase(nodes.find(node->_id.Get()));
        delete node;
    }
}

NodeEditor::~NodeEditor()
{
    ImGuiNodeEditor::DestroyEditor(context);
}

NodeEditorConfig::NodeEditorConfig(std::string saveFile)
    :saveFile(saveFile)
{
}
