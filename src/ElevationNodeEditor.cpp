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
#include <map>


Editor editorM;
static float zoom = 1.0f;
static bool showNodeMaker = false;
FloatValueI* outputNode;

void SetupElevationManager() {
    editorM.context = ImNodes::EditorContextCreate();
    ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
    outputNode = new FloatValueI("Elevation Output");
    outputNode->Setup();
    editorM.nodes.push_back(outputNode);
    ((FloatValueI*)editorM.nodes.back())->outputImage = GetViewportFramebufferColorTextureId();
    editorM.pins.push_back(&((FloatValueI*)editorM.nodes.back())->inputPin);
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


static void ShowNodeMaker() {
    
    if (ImGui::BeginPopupContextWindow()) {
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
        }


        if (ImGui::Button("Float Value"))
        {
            const int node_id = ++editorM.current_id;
            editorM.nodes.push_back(new FloatValueO("Float Value"));
            editorM.pins.push_back(&((FloatValueO*)editorM.nodes.back())->outputPin);
            ((FloatValueO*)editorM.nodes.back())->Setup();
            ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
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
            ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        }
        if (ImGui::Button("Subtract"))
        {
            const int node_id = ++editorM.current_id;
            editorM.nodes.push_back(new SubtractNode());
            editorM.pins.push_back(&((SubtractNode*)editorM.nodes.back())->outputPin);
            editorM.pins.push_back(&((SubtractNode*)editorM.nodes.back())->inputPinX);
            editorM.pins.push_back(&((SubtractNode*)editorM.nodes.back())->inputPinY);
            ((SubtractNode*)editorM.nodes.back())->Setup();
            ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        }
        if (ImGui::Button("Multiply"))
        {
            const int node_id = ++editorM.current_id;
            editorM.nodes.push_back(new MultiplyNode());
            editorM.pins.push_back(&((MultiplyNode*)editorM.nodes.back())->outputPin);
            editorM.pins.push_back(&((MultiplyNode*)editorM.nodes.back())->inputPinX);
            editorM.pins.push_back(&((MultiplyNode*)editorM.nodes.back())->inputPinY);
            ((MultiplyNode*)editorM.nodes.back())->Setup();
            ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        }
        if (ImGui::Button("Divide"))
        {
            const int node_id = ++editorM.current_id;
            editorM.nodes.push_back(new DivideNode());
            editorM.pins.push_back(&((DivideNode*)editorM.nodes.back())->outputPin);
            editorM.pins.push_back(&((DivideNode*)editorM.nodes.back())->inputPinX);
            editorM.pins.push_back(&((DivideNode*)editorM.nodes.back())->inputPinY);
            ((DivideNode*)editorM.nodes.back())->Setup();
            ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
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
            ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        }

        if (ImGui::Button("Cos"))
        {
            editorM.nodes.push_back(new CosNode());
            editorM.pins.push_back(&((CosNode*)editorM.nodes.back())->outputPin);
            editorM.pins.push_back(&((CosNode*)editorM.nodes.back())->inputPin);
            ((CosNode*)editorM.nodes.back())->Setup();
            ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::EndPopup();
    }
}

static void UpdateNodeDeletion() {
    for (Node* node : editorM.nodes) {
        if (ImNodes::IsNodeSelected(node->id)) {
            if (ImGui::GetIO().KeysDown[ImGuiKey_Delete]) {

            }
        }
    }
    
}

static void ShowEditor(const char* editor_name, Editor& editor) {


    if (ImGui::GetIO().KeysDown[ImGuiKey_A]) {
        showNodeMaker = true;
    }
    ImNodes::BeginNodeEditor();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
    ShowNodeMaker();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();


    for (Node* node : editor.nodes)
    {
        node->Render();        
    }

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
}

void ShowElevationNodeEditor(bool* pOpen)
{
    
	ImGui::Begin("Elevation Node Editor", pOpen);

    ShowEditor("editor1", editorM);
	
	ImGui::End();
}


float GetElevation(float x, float y) {
    return outputNode->Evaluate(x, y);
}