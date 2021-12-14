#include "MeshNodeEditor.h"
#include "imgui.h"
#include "Base/ImGuiShapes.h"
#include "Base/NodeEditor/NodeEditor.h"
#include "Utils.h"
#include "imgui_internal.h"

#include "GLFW/glfw3.h"

// Nodes
#include "Nodes/DummyNode.h"
#include "Nodes/AddNode.h"
#include "Nodes/SubNode.h"
#include "Nodes/MulNode.h"
#include "Nodes/DivNode.h"
#include "Nodes/AbsNode.h"
#include "Nodes/SinNode.h"
#include "Nodes/CosNode.h"
#include "Nodes/TanNode.h"
#include "Nodes/MeshCoordinatesNode.h"
#include "Nodes/OutputNode.h"

#include <iostream>
#include <mutex>

#define NODE_MAKER_COND(x) length == 0 || stristr4(x, data) 
#define NODE_MAKER_SHOW(x, y) if (NODE_MAKER_COND(y)) {if (ImGui::Button(y)) { editor->AddNode(new x()); ImGui::CloseCurrentPopup(); } }

static char* stristr4(const char* str, const char* pattern) {
    size_t i;

    if (!*pattern)
        return (char*)str;

    for (; *str; str++) {
        if (toupper((unsigned char)*str) == toupper((unsigned char)*pattern)) {
            for (i = 1;; i++) {
                if (!pattern[i])
                    return (char*)str;
                if (toupper((unsigned char)str[i]) != toupper((unsigned char)pattern[i]))
                    break;
            }
        }
    }
    return NULL;
}

static int* resolution;
static NodeEditor* editor;
static std::string saveVal;
static std::mutex m;

static void ShowNodeMaker() 
{
    char data[1000];
    memset(data, 0, 1000);
    ImGui::InputTextWithHint("##SearchMeshNodes", "Search ...", data, sizeof(data));
    int length = strlen(data);

    NODE_MAKER_SHOW(DummyNode, "Dummy");
    NODE_MAKER_SHOW(MeshCoordinatesNode, "Mesh Coordinates");
    NODE_MAKER_SHOW(AddNode, "Add");
    NODE_MAKER_SHOW(SubNode, "Subtract");
    NODE_MAKER_SHOW(MulNode, "Multiply");
    NODE_MAKER_SHOW(DivNode, "Divide");
    NODE_MAKER_SHOW(AbsNode, "Absolute Value");
    NODE_MAKER_SHOW(SinNode, "Sin");
    NODE_MAKER_SHOW(CosNode, "Cos");
    NODE_MAKER_SHOW(TanNode, "Tan");
   
}

NodeOutput EvaluateMeshNodeEditor(NodeInputParam param)
{
    m.lock();
    NodeOutput o;
    if(editor && editor->outputNode)
        o = editor->outputNode->Evaluate(param, nullptr);
    m.unlock();
    return o;
}

nlohmann::json GetMeshNodeEditorSaveData()
{
    return editor->Save();
}

void SetMeshNodeEditorSaveData(nlohmann::json data)
{
    editor->Load(data);
}

void SetupMeshNodeEditor(int* res)
{
    resolution = res;
    NodeEditorConfig config;
    config.saveFile = GetExecutableDir() + "\\Data\\configs\\meshnodeeditorconfigs.terr3d";
    config.makeNodeFunc = [&]() {
        ImGui::OpenPopup("NodeMakerDropped");
    };
    config.insNodeFunc = [&](nlohmann::json data) -> NodeEditorNode* {
        NodeEditorNode* node;
        MeshNodeEditor::MeshNodeType type = (MeshNodeEditor::MeshNodeType)data["type"];
        switch (type)
        {
        case MeshNodeEditor::MeshNodeType::Dummy:                node = new DummyNode(); break;
        case MeshNodeEditor::MeshNodeType::Output:               node = new OutputNode(); break;
        case MeshNodeEditor::MeshNodeType::MeshCoordinates:      node = new MeshCoordinatesNode(); break;
        case MeshNodeEditor::MeshNodeType::Add:                  node = new AddNode(); break;
        case MeshNodeEditor::MeshNodeType::Sub:                  node = new SubNode(); break;
        case MeshNodeEditor::MeshNodeType::Mul:                  node = new MulNode(); break;
        case MeshNodeEditor::MeshNodeType::Div:                  node = new DivNode(); break;
        case MeshNodeEditor::MeshNodeType::Sin:                  node = new SinNode(); break;
        case MeshNodeEditor::MeshNodeType::Cos:                  node = new CosNode(); break;
        case MeshNodeEditor::MeshNodeType::Tan:                  node = new TanNode(); break;
        case MeshNodeEditor::MeshNodeType::Abs:                  node = new AbsNode(); break;
        default:                                   node = nullptr; Log("Unknown Node Type!"); break;
        }
        return node;
    };
    editor = new NodeEditor(config);
    editor->SetOutputNode(new OutputNode());
    
}

void MeshNodeEditorTick()
{
}

void ShutdownMeshNodeEditor()
{
    if (editor)
        delete editor;
}

void ShowMeshNodeEditor(bool* pOpen)
{
    
    ImGui::Begin("Mesh Node Editor Debugger");
    if (ImGui::Button("Save"))
        saveVal = GetMeshNodeEditorSaveData().dump(4);
    if (ImGui::Button("Load"))
        SetMeshNodeEditorSaveData(nlohmann::json::parse(saveVal));
    if (ImGui::Button("Reset")) {
        editor->Reset();
    }
    ImGui::BeginChild("MNED", ImVec2(400, 500));
    ImGui::Text(saveVal.c_str());
    ImGui::EndChild();
    ImGui::End();
    

    ImGui::Begin("Mesh Node Editor", pOpen);
    if (ImGui::Button("Add Node")) 
    {
        ImGui::OpenPopup("NodeMakerDropped");
    }

    ImGui::Text("WARNING : Work In Progress!");
    
    editor->Render();
    if (ImGui::IsWindowFocused() && (IsKeyDown(TERR3D_KEY_RIGHT_SHIFT) || IsKeyDown(TERR3D_KEY_LEFT_SHIFT)) && IsKeyDown(TERR3D_KEY_A))
    {
        ImGui::OpenPopup("NodeMakerDropped");
    }
    
    if(ImGui::BeginPopup("NodeMakerDropped"))
    {
        ImGui::BeginChild("MNDMP", ImVec2(200, 250));
        ShowNodeMaker();
        ImGui::EndChild();
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
}
