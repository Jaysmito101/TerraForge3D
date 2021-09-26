#define _CRT_SECURE_NO_WARNINGS


#include "ElevationNodeEditor.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <imgui.h>
#include <imnodes/imnodes.h>
#include <NodeEditorNodes.h>
#include <Texture2D.h>
#include <vector>
#include <ViewportFramebuffer.h>
#include <string>
#include <fstream>
#include <map>
#include <Texture2D.h>
#include <GLFW/glfw3.h>
#include <Application.h>

static Editor editorM;
static float zoom = 1.0f;
static bool flag = false;
static bool showHMap = false;
static bool showNodeMaker = false;
static bool drop_make_node = false;
static int drop_midway_start = -1;
static FloatValueI* outputNode;
static int* resolution;
static int currRes;
static float* heightMapData;
static unsigned char* heightMap;
static bool autoUpdate = false;
static Texture2D* hMap;

static std::string ReadrSourceFile(std::string path, bool* result) {
    std::fstream newfile;
    newfile.open(path.c_str(), std::ios::in);
    if (newfile.is_open()) {
        std::string tp;
        std::string res = "";
        getline(newfile, res, '\0');
        newfile.close();
        return res;
    }
    else {
        *result = false;
    }
    return std::string("");
}

static void SetUpHeightMap(int res) {
    if (heightMapData)
        delete[] heightMapData;
    if (heightMap)
        delete[] heightMap;
    if (hMap)
        delete hMap;
    hMap = new Texture2D(currRes, currRes);
    heightMapData = new float[res * res];
    memset(heightMapData, 0, res*res);
    heightMap = new unsigned char[res * res * 3];
    memset(heightMap, 0, res * res * 3);
}

static void UpdateHeightMap() {
    float t = 0;
    for (int i = 0; i < currRes; i++) {
        for (int j = 0; j < currRes; j++) {
            heightMapData[i * currRes + j] = outputNode->Evaluate(i, j);
            t = heightMapData[i * currRes + j] * 0.5f + 0.5f;
            heightMap[i * currRes * 3 + j * 3 + 0] = (unsigned char)(t * 255);
            heightMap[i * currRes * 3 + j * 3 + 1] = (unsigned char)(t * 255);
            heightMap[i * currRes * 3 + j * 3 + 2] = (unsigned char)(t * 255);
        }
    }
    hMap->SetData(heightMap, currRes*currRes*3);
}


void SetupElevationManager(int* res) {
    resolution = res;
    currRes = *resolution;
    SetUpHeightMap(currRes);
    editorM.context = ImNodes::EditorContextCreate();
    ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
    outputNode = new FloatValueI("Elevation Output");
    outputNode->Setup();
    outputNode->outputTex = GetViewportFramebufferColorTextureId();
    editorM.pins.push_back(&(outputNode)->inputPin);
    UpdateHeightMap();
}

void ShutdownElevationNodeEditor() {
    
}

static void UpdateLinkCreation(Editor& editor) {

    {
        Link link;
        if (ImNodes::IsLinkCreated(&link.start_attr, &link.end_attr))
        {
            link.id = ++editor.current_id;
            FloatPin* st = (FloatPin*)editor.FindPin(link.start_attr);
            FloatPin* en = (FloatPin*)editor.FindPin(link.end_attr);
            bool flag = true;
            {
                if (st->isLinked || en->isLinked) {
                    return;
                }
                st->isLinked = true;
                link.other = en;
                st->link = link;
            }
            {
                if (en->isLinked) {
                    return;
                }
                en->isLinked = true;
                link.other = st;
                en->link = link;
            }
            if (flag)
                editor.links.push_back(link);
        }
    }

}

static void MakeLink(Pin* start, Pin* end) {
    Link lnk;
    lnk.start_attr = start->id;
    lnk.end_attr = end->id;
    start->isLinked = true;
    lnk.other = end;
    start->link = Link(lnk);
    end->isLinked = true;
    lnk.other = start;
    end->link = Link(lnk);
    editorM.links.push_back(lnk);
    drop_midway_start = -1;
}

static Node* MakeNode(nlohmann::json& nodedata) {
    if (nodedata["type"] == NodeType::FloatNodeI) {
        FloatValueI* nd = new FloatValueI();
        nd->Load(nodedata);
        return nd;
    }

    if (nodedata["type"] == NodeType::FloatNodeO) {
        FloatValueO* nd = new FloatValueO();
        nd->Load(nodedata);
        return nd;
    }

    if (nodedata["type"] == NodeType::MeshCoord) {
        MeshCoordNode* nd = new MeshCoordNode();
        nd->Load(nodedata);
        return nd;
    }

    if (nodedata["type"] == NodeType::Add) {
        AdderNode* nd = new AdderNode();
        nd->Load(nodedata);
        return nd;
    }

    if (nodedata["type"] == NodeType::Sub) {
        SubtractNode* nd = new SubtractNode();
        nd->Load(nodedata);
        return nd;
    }

    if (nodedata["type"] == NodeType::Div) {
        DivideNode* nd = new DivideNode();
        nd->Load(nodedata);
        return nd;
    }

    if (nodedata["type"] == NodeType::Mul) {
        MultiplyNode* nd = new MultiplyNode();
        nd->Load(nodedata);
        return nd;
    }

    if (nodedata["type"] == NodeType::Sin) {
        SinNode* nd = new SinNode();
        nd->Load(nodedata);
        return nd;
    }

    if (nodedata["type"] == NodeType::Cos) {
        CosNode* nd = new CosNode();
        nd->Load(nodedata);
        return nd;
    }
}

static void FixPinPointers() {
    for (Pin* p : editorM.pins) {
        int st = p->link.start_attr;
        int en = p->link.end_attr;
        int oid;
        oid = st;
        if (st == p->link.id)
            oid = en;
        Pin* otherPin = editorM.FindPin(oid);
        p->link.other = otherPin;
    }
}

static void LoadEditorData(Editor& editor, nlohmann::json data) {
    ImNodes::LoadEditorStateFromIniString(editor.context, data["context"].dump().c_str(), data["context"].dump().size());
    for (Node* n : editor.nodes) {
        if (n->id == outputNode->id)
            continue;
        delete n;
    }
    editor.nodes = std::vector<Node*>();
    for (nlohmann::json node : data["nodes"]) {
        editor.nodes.push_back(MakeNode(node));
    }


    editor.pins = std::vector<Pin*>();
    for (Node* n : editor.nodes) {
        std::vector<void*> np = n->GetPins();
        for (void* pn : np) {
            editor.pins.push_back((Pin*)pn);
        }
    }
    editor.pins.push_back(&(outputNode)->inputPin);


    editor.links = std::vector<Link>();
    for (nlohmann::json link : data["links"]) {
        editor.links.push_back(Link());
        editor.links.back().Load(link);
        if (editor.links.back().end_attr == outputNode->inputPin.id) {
            outputNode->inputPin.isLinked = true;
            outputNode->inputPin.link = editor.links.back();
            outputNode->inputPin.link.other = editor.FindPin(outputNode->inputPin.link.start_attr);
        }
    }
    FixPinPointers();

    ImNodes::EditorContextSet(editor.context);
    for (nlohmann::json nodePos : data["nodePositions"]) {
        ImNodes::SetNodeEditorSpacePos(nodePos["id"], ImVec2(nodePos["x"], nodePos["y"]));
    }
}

void SetElevationNodeEditorSaveData(nlohmann::json data) {
    LoadEditorData(editorM, data);
}

static void ResetNodeEditor() {
    ImNodes::EditorContextFree(editorM.context);
    editorM.context = ImNodes::EditorContextCreate();
    delete outputNode;
    outputNode = new FloatValueI("Elevation Output");
    outputNode->Setup();
    for (Node* n : editorM.nodes) {
        delete n;
    }
    editorM.nodes = std::vector<Node*>();
    editorM.links = std::vector<Link>();
    editorM.pins = std::vector<Pin*>();
    editorM.pins.push_back(&(outputNode)->inputPin);
}

nlohmann::json GetElevationNodeEditorSaveData() {
    return editorM.Save(outputNode);
}

static void ShowNodeMaker(Pin* start_drop) {

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_PopupBg]);
    ImGui::BeginChild("NodeMakerPopup", ImVec2(200, 200));

    ImGui::Separator();
    ImGui::Text("Values");
    ImGui::Separator();

    if (ImGui::Button("Mesh Coordinates"))
    {
        const int node_id = ++editorM.current_id;
        editorM.nodes.push_back(new MeshCoordNode());
        editorM.pins.push_back(&((MeshCoordNode*)editorM.nodes.back())->outputPinX);
        editorM.pins.push_back(&((MeshCoordNode*)editorM.nodes.back())->outputPinY);
        ((MeshCoordNode*)editorM.nodes.back())->Setup();
        ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        ImGui::CloseCurrentPopup();
    }


    if (ImGui::Button("Float Value"))
    {
        const int node_id = ++editorM.current_id;
        editorM.nodes.push_back(new FloatValueO("Float Value"));
        editorM.pins.push_back(&((FloatValueO*)editorM.nodes.back())->outputPin);
        ((FloatValueO*)editorM.nodes.back())->Setup();
        ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        ImGui::CloseCurrentPopup();
    }

    ImGui::Separator();
    ImGui::Text("Math Nodes");
    ImGui::Separator();

    if (ImGui::Button("Add"))
    {
        const int node_id = ++editorM.current_id;
        editorM.nodes.push_back(new AdderNode());
        editorM.pins.push_back(&((AdderNode*)editorM.nodes.back())->outputPin);
        editorM.pins.push_back(&((AdderNode*)editorM.nodes.back())->inputPinX);
        editorM.pins.push_back(&((AdderNode*)editorM.nodes.back())->inputPinY);
        ((AdderNode*)editorM.nodes.back())->Setup();
        if (start_drop) {
            MakeLink(start_drop, &((AdderNode*)editorM.nodes.back())->inputPinX);
        }
        ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button("Subtract"))
    {
        const int node_id = ++editorM.current_id;
        editorM.nodes.push_back(new SubtractNode());
        editorM.pins.push_back(&((SubtractNode*)editorM.nodes.back())->outputPin);
        editorM.pins.push_back(&((SubtractNode*)editorM.nodes.back())->inputPinX);
        editorM.pins.push_back(&((SubtractNode*)editorM.nodes.back())->inputPinY);
        ((SubtractNode*)editorM.nodes.back())->Setup();
        if (start_drop) {
            MakeLink(start_drop, &((SubtractNode*)editorM.nodes.back())->inputPinX);
        }
        ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button("Multiply"))
    {
        const int node_id = ++editorM.current_id;
        editorM.nodes.push_back(new MultiplyNode());
        editorM.pins.push_back(&((MultiplyNode*)editorM.nodes.back())->outputPin);
        editorM.pins.push_back(&((MultiplyNode*)editorM.nodes.back())->inputPinX);
        editorM.pins.push_back(&((MultiplyNode*)editorM.nodes.back())->inputPinY);
        ((MultiplyNode*)editorM.nodes.back())->Setup();
        if (start_drop) {
            MakeLink(start_drop, &((MultiplyNode*)editorM.nodes.back())->inputPinX);
        }
        ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button("Divide"))
    {
        const int node_id = ++editorM.current_id;
        editorM.nodes.push_back(new DivideNode());
        editorM.pins.push_back(&((DivideNode*)editorM.nodes.back())->outputPin);
        editorM.pins.push_back(&((DivideNode*)editorM.nodes.back())->inputPinX);
        editorM.pins.push_back(&((DivideNode*)editorM.nodes.back())->inputPinY);
        ((DivideNode*)editorM.nodes.back())->Setup();
        if (start_drop) {
            MakeLink(start_drop, &((DivideNode*)editorM.nodes.back())->inputPinX);
        }
        ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        ImGui::CloseCurrentPopup();
    }

    ImGui::Separator();
    ImGui::Text("Math Functions");
    ImGui::Separator();

    if (ImGui::Button("Sin"))
    {
        editorM.nodes.push_back(new SinNode());
        editorM.pins.push_back(&((SinNode*)editorM.nodes.back())->outputPin);
        editorM.pins.push_back(&((SinNode*)editorM.nodes.back())->inputPin);
        ((SinNode*)editorM.nodes.back())->Setup();
        if (start_drop) {
            MakeLink(start_drop, &((SinNode*)editorM.nodes.back())->inputPin);
        }
        ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        ImGui::CloseCurrentPopup();
    }

    if (ImGui::Button("Cos"))
    {
        editorM.nodes.push_back(new CosNode());
        editorM.pins.push_back(&((CosNode*)editorM.nodes.back())->outputPin);
        editorM.pins.push_back(&((CosNode*)editorM.nodes.back())->inputPin);
        ((CosNode*)editorM.nodes.back())->Setup();
        if (start_drop) {
            MakeLink(start_drop, &((CosNode*)editorM.nodes.back())->inputPin);
        }
        ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    
}

static void DeletePins(std::vector<void*> pins) {
    
    for (void* pp : pins) {
        Pin* p = (Pin*)pp;
        if (p) {
            if (p->link.other) {
                ((Pin*)p->link.other)->isLinked = false;

                std::vector<Link>::iterator positionL = std::find(editorM.links.begin(), editorM.links.end(), p->link);
                if (positionL != editorM.links.end()) // == myVector.end() means the element was not found
                    editorM.links.erase(positionL);

                std::vector<Pin*>::iterator position = std::find(editorM.pins.begin(), editorM.pins.end(), p);
                if (position != editorM.pins.end()) // == myVector.end() means the element was not found
                    editorM.pins.erase(position);
            }
        }
    }

}

static void UpdateNodeDeletion() {

    for (Node* node : editorM.nodes) {
        if (ImNodes::IsNodeSelected(node->id)) {
            if (node->id == outputNode->id)
                return;
            if (glfwGetKey(Application::Get()->GetWindow()->GetNativeWindow(), GLFW_KEY_DELETE)) {
                //editorM.nodes.erase(std::remove(editorM.nodes.begin(), editorM.nodes.end(), 8), editorM.nodes.end());
                DeletePins(node->GetPins());
                std::vector<Node*>::iterator position = std::find(editorM.nodes.begin(), editorM.nodes.end(), node);
                if (position != editorM.nodes.end()) // == myVector.end() means the element was not found
                    editorM.nodes.erase(position);
            }
        }
    }
    
}

static void ShowEditor(const char* editor_name, Editor& editor) {
    ImGui::Checkbox("Drop To Make Node (Beta)", &drop_make_node);
    ImGui::Checkbox("Auto Update Node Output", &autoUpdate);
    if (ImGui::Button("Reset Node Editor"))
        ResetNodeEditor();
    ImGui::SameLine();
    if (ImGui::Button("Update Node Output"))
        UpdateHeightMap();
    ImGui::SameLine();
    if (!showHMap) {
        if (ImGui::Button("Show Heightmap on Output Node")) {
            if (outputNode) {
                outputNode->outputTex = hMap->GetRendererID();
                showHMap = true;
            }
        }
    }else{
        if (ImGui::Button("Show Viewport on Output Node")) {
            if (outputNode) {
                outputNode->outputTex = GetViewportFramebufferColorTextureId();
                showHMap = false;
            }
        }
    }

    if (ImGui::GetIO().KeysDown[ImGuiKey_A]) {
        showNodeMaker = true;
    }

    ImNodes::EditorContextSet(editorM.context);

    ImNodes::BeginNodeEditor();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
    if (ImGui::BeginPopupContextWindow()) {
        ShowNodeMaker(0);
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();


    for (Node* node : editor.nodes)
    {
        node->Render();        
    }

    outputNode->Render();

    for (const Link& link : editor.links)
    {
        ImNodes::Link(link.id, link.start_attr, link.end_attr);
    }

    ImNodes::EndNodeEditor();

    UpdateLinkCreation(editor);
    

    {
        int link_id;
        if (ImNodes::IsLinkDestroyed(&link_id))
        {
            auto iter = std::find_if(
                editor.links.begin(), editor.links.end(), [link_id](const Link& link) -> bool {
                    return link.id == link_id;
                });
            assert(iter != editor.links.end());
            FloatPin* st = (FloatPin*)editor.FindPin(iter->start_attr);
            {
                st->isLinked = false;
            }
            FloatPin* en = (FloatPin*)editor.FindPin(iter->end_attr);
            {
                en->isLinked = false;
            }
            editor.links.erase(iter);
        }
    }

    UpdateNodeDeletion();

    if (drop_make_node) {
        int start_id;
        if (ImNodes::IsLinkDropped(&start_id, false)) {
            drop_midway_start = start_id;
            ImGui::OpenPopup("NodeMakerDropped");
        }
    }

    if (ImGui::BeginPopup("NodeMakerDropped") && drop_midway_start >= 0)
    {
        if (drop_midway_start == outputNode->inputPin.id) {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
        ShowNodeMaker(editorM.FindPin(drop_midway_start));
        ImGui::EndPopup();
    }
}

void ShowElevationNodeEditor(bool* pOpen)
{
    if (currRes != *resolution) {
        currRes = *resolution;
        SetUpHeightMap(currRes);
        UpdateHeightMap();
    }
    if (autoUpdate) {
        UpdateHeightMap();
    }
	ImGui::Begin("Elevation Node Editor", pOpen);

    ShowEditor("editor1", editorM);
	
	ImGui::End();
}

float GetElevation(float x, float y) {
    return heightMapData[(int)x*currRes+(int)y];
}