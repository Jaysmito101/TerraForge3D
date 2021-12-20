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
#include "Nodes/BlendNode.h"
#include "Nodes/CurveNode.h"
#include "Nodes/NoisePerlinNode.h"
#include "Nodes/NoiseCellularNode.h"
#include "Nodes/NoiseValueNode.h"
#include "Nodes/NoiseValueCubicNode.h"
#include "Nodes/NoiseOpenSimplex2Node.h"
#include "Nodes/NoiseOpenSimplex2SNode.h"
#include "Nodes/MeshCoordinatesNode.h"
#include "Nodes/MinMeshCoordinatesNode.h"
#include "Nodes/MaxMeshCoordinatesNode.h"
#include "Nodes/TextureCoordinatesNode.h"
#include "Nodes/TimeBasedSeedNode.h"
#include "Nodes/RandomNumberNode.h"
#include "Nodes/DuplicateNode.h"
#include "Nodes/MathFunctionNode.h"
#include "Nodes/ModuleNode.h"
#include "Nodes/OutputNode.h"
#include "Nodes/PixelateNode.h"
#include "Nodes/VisualizerNode.h"
#include "Nodes/TextureNode.h"
#include "Nodes/HillNode.h"

#include "Modules/ModuleManager.h"

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

static ModuleManager* modMan;
static NodeEditor* editor;
static std::string saveVal;
static std::mutex m;

static void ShowNodeMaker() 
{
    static char data[1000];
    ImGui::InputTextWithHint("##SearchMeshNodes", "Search ...", data, sizeof(data));
    int length = strlen(data);

    NODE_MAKER_SHOW(DummyNode, "Dummy");
    NODE_MAKER_SHOW(MeshCoordinatesNode, "Mesh Coordinates");
    NODE_MAKER_SHOW(MaxMeshCoordinatesNode, "Maximum Mesh Coordinates");
    NODE_MAKER_SHOW(TextureCoordinatesNode, "Texture Coordinates");
    NODE_MAKER_SHOW(MinMeshCoordinatesNode, "Minimum Mesh Coordinates");
    NODE_MAKER_SHOW(TimeBasedSeedNode, "Time Based Seed");
    NODE_MAKER_SHOW(PixelateNode, "Pixelate");
    NODE_MAKER_SHOW(TextureNode, "Texture");
    NODE_MAKER_SHOW(HillNode, "Hill");
    NODE_MAKER_SHOW(AddNode, "Add");
    NODE_MAKER_SHOW(SubNode, "Subtract");
    NODE_MAKER_SHOW(MulNode, "Multiply");
    NODE_MAKER_SHOW(DivNode, "Divide");
    NODE_MAKER_SHOW(AbsNode, "Absolute Value");
    NODE_MAKER_SHOW(SinNode, "Sin");
    NODE_MAKER_SHOW(CosNode, "Cos");
    NODE_MAKER_SHOW(TanNode, "Tan");
    NODE_MAKER_SHOW(AbsNode, "Absolute Value");
    NODE_MAKER_SHOW(BlendNode, "Blend");
    NODE_MAKER_SHOW(CurveNode, "Curve Editor");
    NODE_MAKER_SHOW(RandomNumberNode, "Random Number");
    NODE_MAKER_SHOW(NoisePerlinNode, "Perlin Noise");
    NODE_MAKER_SHOW(NoiseCellularNode, "Cellular Noise");
    NODE_MAKER_SHOW(NoiseValueNode, "Value Noise");
    NODE_MAKER_SHOW(NoiseOpenSimplex2Node, "Open Simplex 2 Noise");
    NODE_MAKER_SHOW(NoiseOpenSimplex2SNode, "Open Simplex 2S Noise");
    NODE_MAKER_SHOW(NoiseValueCubicNode, "Value Cubic Noise");
    NODE_MAKER_SHOW(DuplicateNode, "Duplicate");
    NODE_MAKER_SHOW(MathFunctionNode, "Custom Math Funcion");
    NODE_MAKER_SHOW(VisualizerNode, "Visualizer");

    for (NodeModule* mod : modMan->noModules)
    {
        if (NODE_MAKER_COND(mod->name.data())) { 
            if (ImGui::Button(mod->name.data())) {
                editor->AddNode(new ModuleNode(mod->id, mod));
                ImGui::CloseCurrentPopup(); 
            }
        }
    }
   
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

void SetupMeshNodeEditor(ModuleManager* m)
{
    modMan = m;
    NodeEditorConfig config;
    config.saveFile = GetExecutableDir() + "\\Data\\configs\\meshnodeeditorconfigs.terr3d";
    config.makeNodeFunc = [&]() {
        ImGuiNodeEditor::Suspend();
        ImGui::OpenPopup("NodeMakerDropped");
        ImGuiNodeEditor::Resume();
    };
    config.insNodeFunc = [&](nlohmann::json data) -> NodeEditorNode* {
        NodeEditorNode* node;
        MeshNodeEditor::MeshNodeType type = (MeshNodeEditor::MeshNodeType)data["type"];
        switch (type)
        {
        case MeshNodeEditor::MeshNodeType::Dummy:                node = new DummyNode(); break;
        case MeshNodeEditor::MeshNodeType::Output:               node = new OutputNode(); break;
        case MeshNodeEditor::MeshNodeType::MeshCoordinates:      node = new MeshCoordinatesNode(); break;
        case MeshNodeEditor::MeshNodeType::MaxMeshCoordinates:   node = new MaxMeshCoordinatesNode(); break;
        case MeshNodeEditor::MeshNodeType::MinMeshCoordinates:   node = new MinMeshCoordinatesNode(); break;
        case MeshNodeEditor::MeshNodeType::TextureCoordinates:   node = new TextureCoordinatesNode(); break;
        case MeshNodeEditor::MeshNodeType::TimeBasedSeed:        node = new TimeBasedSeedNode(); break;
        case MeshNodeEditor::MeshNodeType::Duplicate:            node = new DuplicateNode(); break;
        case MeshNodeEditor::MeshNodeType::Add:                  node = new AddNode(); break;
        case MeshNodeEditor::MeshNodeType::Sub:                  node = new SubNode(); break;
        case MeshNodeEditor::MeshNodeType::Mul:                  node = new MulNode(); break;
        case MeshNodeEditor::MeshNodeType::Div:                  node = new DivNode(); break;
        case MeshNodeEditor::MeshNodeType::Sin:                  node = new SinNode(); break;
        case MeshNodeEditor::MeshNodeType::Cos:                  node = new CosNode(); break;
        case MeshNodeEditor::MeshNodeType::Tan:                  node = new TanNode(); break;
        case MeshNodeEditor::MeshNodeType::Abs:                  node = new AbsNode(); break;
        case MeshNodeEditor::MeshNodeType::Blend:                node = new BlendNode(); break;
        case MeshNodeEditor::MeshNodeType::Curve:                node = new CurveNode(); break;
        case MeshNodeEditor::MeshNodeType::RandomNumber:         node = new RandomNumberNode(); break;
        case MeshNodeEditor::MeshNodeType::NoisePerlin:          node = new NoisePerlinNode(); break;
        case MeshNodeEditor::MeshNodeType::NoiseCellular:        node = new NoiseCellularNode(); break;
        case MeshNodeEditor::MeshNodeType::NoiseOpenSimplex2:    node = new NoiseOpenSimplex2Node(); break;
        case MeshNodeEditor::MeshNodeType::NoiseOpenSimplex2S:   node = new NoiseOpenSimplex2SNode(); break;
        case MeshNodeEditor::MeshNodeType::NoiseValue:           node = new NoiseValueNode(); break;
        case MeshNodeEditor::MeshNodeType::NoiseValueCubic:      node = new NoiseValueCubicNode(); break;
        case MeshNodeEditor::MeshNodeType::MathFunction:         node = new MathFunctionNode(); break;
        case MeshNodeEditor::MeshNodeType::Texture:              node = new TextureNode(); break;
        case MeshNodeEditor::MeshNodeType::Pixelate:             node = new PixelateNode(); break;
        case MeshNodeEditor::MeshNodeType::Visualizer:           node = new VisualizerNode(); break;
        case MeshNodeEditor::MeshNodeType::Hill:                 node = new HillNode(); break;
        case MeshNodeEditor::MeshNodeType::Module:               node = new ModuleNode(data["mid"], modMan->FindNodeModule(data["mid"])); break;
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
    /*
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
    */

    ImGui::Begin("Mesh Node Editor", pOpen);
    if (ImGui::Button("Add Node")) 
    {
        ImGui::OpenPopup("NodeMakerDropped");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
        editor->Reset();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export"))
    {
        std::string file = ShowSaveFileDialog("*.terr3d");
        if (file.size() > 3)
        {
            if (file.find(".terr3d") == std::string::npos)
                file += ".terr3d";
            SaveToFile(file, editor->Save().dump(4));
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import"))
    {
        std::string file = ShowOpenFileDialog("*.terr3d");
        if (file.size() > 3)
        {
            bool tmp = false;
            editor->Reset();
            editor->Load(nlohmann::json::parse(ReadShaderSourceFile(file, &tmp)));
        }
    }


    editor->Render();

    if (ImGui::IsWindowFocused() && (((IsKeyDown(TERR3D_KEY_RIGHT_SHIFT) || IsKeyDown(TERR3D_KEY_LEFT_SHIFT)) && IsKeyDown(TERR3D_KEY_A)) || IsMouseButtonDown(TERR3D_MOUSE_BUTTON_MIDDLE)))
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
