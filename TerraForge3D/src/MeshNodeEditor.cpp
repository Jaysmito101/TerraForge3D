#include "MeshNodeEditor.h"
#include "imgui.h"
#include "Base/ImGuiShapes.h"
#include "Base/NodeEditor/NodeEditor.h"
#include "Utils.h"
#include "imgui_internal.h"

#include "GLFW/glfw3.h"

// Nodes
#include "Nodes/DummyNode.h"
#include "Nodes/OutputNode.h"

#include <iostream>

#define NODE_MAKER_COND(x) length == 0 || stristr4(data, x) 
#define NODE_MAKER_SHOW(x) if(ImGui::Button(#x)){editor->AddNode(new x()); ImGui::CloseCurrentPopup();}

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

static void ShowNodeMaker() 
{
    char data[1000];
    memset(data, 0, 1000);
    ImGui::InputTextWithHint("##SearchMeshNodes", "Search ...", data, sizeof(data));
    int length = strlen(data);

    if (NODE_MAKER_COND("Dummy")) {
        NODE_MAKER_SHOW(DummyNode);
    }
    else if (NODE_MAKER_COND("Add")) {
        ImGui::CloseCurrentPopup();
    }
}

MeshNodeEditorResult EvaluateMeshNodeEditor(MeshNodeEditorParam param)
{
    NodeInputParam iParam;
    iParam.x = param.x;
    iParam.y = param.y;
    iParam.z = param.z;
    MeshNodeEditorResult res;
    if(editor->outputNode)
        res.value = editor->outputNode->Evaluate(iParam).value;
    return res;
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
        case MeshNodeEditor::Dummy:                node = new DummyNode(); break;
        case MeshNodeEditor::Output:               node = new OutputNode(); break;
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
        ImGui::BeginChild("MNDMP", ImVec2(150, 200));
        ShowNodeMaker();
        ImGui::EndChild();
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
}
