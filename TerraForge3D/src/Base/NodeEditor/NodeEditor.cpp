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

void NodeEditorLink::Load(nlohmann::json data)
{
    color.x = data["color"]["r"];
    color.y = data["color"]["g"];
    color.z = data["color"]["b"];
    color.w = data["color"]["a"];
    thickness = data["thickness"];
    id = data["id"];
    _id = id;
}

nlohmann::json NodeEditorPin::Save()
{
    nlohmann::json data;
    data["id"] = id;
    data["type"] = type;
    data["color"] = color;
    return data;
}

void NodeEditorPin::Load(nlohmann::json data)
{
    id = data["id"];
    _id = id;
    type = data["type"];
    color = data["color"];
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
    if (lnk->from->parent->id == lnk->to->parent->id)
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

void NodeEditorNode::LoadInternal(nlohmann::json data)
{
    name = data["name"];
    headerColor = data["headerColor"];
    id = data["id"];
    _id = id;
    for (nlohmann::json pns : data["inputPins"])
        inputPins[pns["index"]]->Load(pns);
    for (nlohmann::json pns : data["outputPins"])
        outputPins[pns["index"]]->Load(pns);
    Load(data);
}

nlohmann::json NodeEditorNode::SaveInternal()
{
    nlohmann::json data = Save();
    data["name"] = name;
    data["headerColor"] = headerColor;
    data["id"] = id;
    nlohmann::json tmp, tmp2;
    for (int i = 0; i < inputPins.size(); i++) {
        tmp2 = inputPins[i]->Save();
        tmp2["index"] = i;
        tmp.push_back(tmp2);
    }
    data["inputPins"] = tmp;
    tmp.clear();
    for (int i = 0; i < outputPins.size(); i++) {
        tmp2 = outputPins[i]->Save();
        tmp2["index"] = i;
        tmp.push_back(tmp2);
    }
    data["outputPins"] = tmp;
    return data;
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
       if(it.second->name != "Output")
           tmp.push_back(it.second->SaveInternal());
   }
   data["nodes"] = tmp;
   tmp = nlohmann::json();
   for (auto& it : links) {
       tmp.push_back(it.second->Save());
   }
   data["links"] = tmp;
   data["uidSeed"] = GenerateUID();
   return data;
}

void NodeEditor::Load(nlohmann::json data)
{
    if (!config.insNodeFunc)
        throw std::runtime_error("Node Instantiate Function Not Found!");
    Reset();
    SeUIDSeed(data["uidSeed"]);
    for (nlohmann::json ndata : data["nodes"])
    {
        NodeEditorNode* nd = (config.insNodeFunc(ndata));
        nd->LoadInternal(ndata);
        AddNode(nd);
    }

    for (nlohmann::json ldata : data["links"])
    {
        NodeEditorLink* lnk = new NodeEditorLink();
        lnk->Load(ldata);
        ImGuiNodeEditor::PinId fromId, toId;
        fromId = (int)ldata["from"];
        toId = (int)ldata["to"];
        lnk->from = pins[fromId.Get()];
        lnk->to = pins[toId.Get()];
        lnk->from->Link(lnk);
        lnk->to->Link(lnk);
        links[lnk->_id.Get()] = lnk;
    }
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
    outputNode->Render();

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
                NodeEditorPin* pin = pins[iPid.Get()];
                if(pin->IsLinked())
                    DeleteLink(pin->link);
            }
            else if (oPid) {
                NodeEditorPin* pin = pins[oPid.Get()];
                if (pin->IsLinked())
                    DeleteLink(pin->link);
            }
        }

        ImGuiNodeEditor::PinId nPid;
        if (config.makeNodeFunc && ImGuiNodeEditor::QueryNewNode(&nPid))
        {
            if (ImGuiNodeEditor::AcceptNewItem())
            {
                NodeEditorPin* pin = pins[nPid.Get()];
                if (pin->IsLinked())
                    DeleteLink(pin->link);
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
                    DeleteLink(link);
                }
            }
        }
    }
    ImGuiNodeEditor::EndDelete();

    if (config.updateFunc)
        config.updateFunc();
    ImGuiNodeEditor::End();
    ImGuiNodeEditor::SetCurrentEditor(nullptr);
}

void NodeEditor::DeleteLink(NodeEditorLink* link)
{
    link->from->link = nullptr;
    link->to->link = nullptr;
    links.erase(links.find(link->_id.Get()));
    delete link;
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

void NodeEditor::Reset()
{
    // Clear Up Node Editor
    int size = 0;
    std::unordered_map<uintptr_t, NodeEditorNode*> nodesc = nodes;
    for (auto& it : nodesc)
        DeleteNode(it.second);
    NodeEditorNode* node = outputNode;
    for (auto& it : node->GetPins())
    {
        pins[it->_id.Get()] = it;
    }
    node->Setup();
}

void NodeEditor::SetOutputNode(NodeEditorNode* node)
{
    outputNode = node;
    for (auto& it : node->GetPins())
    {
        pins[it->_id.Get()] = it;
    }
    node->Setup();
}


NodeEditorConfig::NodeEditorConfig(std::string saveFile)
    :saveFile(saveFile)
{
}
