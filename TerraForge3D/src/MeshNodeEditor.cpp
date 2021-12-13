#include "MeshNodeEditor.h"
#include "imgui.h"
#include "Base/ImGuiShapes.h"
#include "Base/NodeEditor/NodeEditor.h"
#include "Utils.h"
#include "imgui_internal.h"

// Nodes
#include "Nodes/DummyNode.h"

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
    return MeshNodeEditorResult();
}

nlohmann::json GetMeshNodeEditorSaveData()
{
    return nlohmann::json();
}

void SetMeshNodeEditorSaveData(nlohmann::json data)
{
}

void SetupMeshNodeEditor(int* res)
{
    resolution = res;
    NodeEditorConfig config;
    config.saveFile = GetExecutableDir() + "\\Data\\configs\\meshnodeeditorconfigs.terr3d";
    config.makeNodeFunc = []() {
        ImGui::OpenPopup("NodeMakerDropped");
    };
    editor = new NodeEditor(config);
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
    ImGui::Begin("Mesh Node Editor", pOpen);
    if (ImGui::Button("Add Node")) 
    {
        editor->AddNode(new DummyNode());
    }

    ImGui::Text("WARNING : Work In Progress!");
    
    editor->Render();

    if (ImGui::BeginPopupContextWindow("Mesh Node Maker"))
    {
        ShowNodeMaker();

        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if(ImGui::BeginPopup("NodeMakerDropped"))
    {
        ShowNodeMaker();
        ImGui::EndPopup();
    }

    ImGui::End();
}
