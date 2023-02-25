#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "Utils/Utils.h"
#include "Platform.h"

#include "imgui.h"
#include "Base/ImGuiShapes.h"
#include "Base/NodeEditor/NodeEditor.h"
#include "imgui_internal.h"

#include "GLFW/glfw3.h"

// Nodes
#include "Generators/CPUNodeEditor/Nodes/DummyNode.h"
#include "Generators/CPUNodeEditor/Nodes/AddNode.h"
#include "Generators/CPUNodeEditor/Nodes/SubNode.h"
#include "Generators/CPUNodeEditor/Nodes/MulNode.h"
#include "Generators/CPUNodeEditor/Nodes/DivNode.h"
#include "Generators/CPUNodeEditor/Nodes/AbsNode.h"
#include "Generators/CPUNodeEditor/Nodes/SinNode.h"
#include "Generators/CPUNodeEditor/Nodes/CosNode.h"
#include "Generators/CPUNodeEditor/Nodes/TanNode.h"
#include "Generators/CPUNodeEditor/Nodes/BlendNode.h"
#include "Generators/CPUNodeEditor/Nodes/CurveNode.h"
#include "Generators/CPUNodeEditor/Nodes/NoisePerlinNode.h"
#include "Generators/CPUNodeEditor/Nodes/NoiseCellularNode.h"
#include "Generators/CPUNodeEditor/Nodes/NoiseValueNode.h"
#include "Generators/CPUNodeEditor/Nodes/NoiseValueCubicNode.h"
#include "Generators/CPUNodeEditor/Nodes/NoiseOpenSimplex2Node.h"
#include "Generators/CPUNodeEditor/Nodes/NoiseOpenSimplex2SNode.h"
#include "Generators/CPUNodeEditor/Nodes/MeshCoordinatesNode.h"
#include "Generators/CPUNodeEditor/Nodes/MinMeshCoordinatesNode.h"
#include "Generators/CPUNodeEditor/Nodes/MaxMeshCoordinatesNode.h"
#include "Generators/CPUNodeEditor/Nodes/TextureCoordinatesNode.h"
#include "Generators/CPUNodeEditor/Nodes/TimeBasedSeedNode.h"
#include "Generators/CPUNodeEditor/Nodes/RandomNumberNode.h"
#include "Generators/CPUNodeEditor/Nodes/DuplicateNode.h"
#include "Generators/CPUNodeEditor/Nodes/MathFunctionNode.h"
#include "Generators/CPUNodeEditor/Nodes/OutputNode.h"
#include "Generators/CPUNodeEditor/Nodes/PixelateNode.h"
#include "Generators/CPUNodeEditor/Nodes/VisualizerNode.h"
#include "Generators/CPUNodeEditor/Nodes/TextureNode.h"
#include "Generators/CPUNodeEditor/Nodes/HeightmapNode.h"
#include "Generators/CPUNodeEditor/Nodes/HillNode.h"
#include "Generators/CPUNodeEditor/Nodes/MinValNode.h"
#include "Generators/CPUNodeEditor/Nodes/SquareNode.h"
#include "Generators/CPUNodeEditor/Nodes/ClampNode.h"


#include <iostream>
#include <mutex>

#ifdef min 
#undef min
#endif
#ifdef max
#undef max
#endif

static char *stristr4(const char *str, const char *pattern)
{
	size_t i;

	if (!*pattern)
	{
		return (char *)str;
	}

	for (; *str; str++)
	{
		if (toupper((unsigned char)*str) == toupper((unsigned char)*pattern))
		{
			for (i = 1;; i++)
			{
				if (!pattern[i])
				{
					return (char *)str;
				}

				if (toupper((unsigned char)str[i]) != toupper((unsigned char)pattern[i]))
				{
					break;
				}
			}
		}
	}

	return NULL;
}

#define NODE_MAKER_COND(x) length == 0 || stristr4(x, data)
#define NODE_MAKER_SHOW(x, y) if (NODE_MAKER_COND(y)) {if (ImGui::Button((y + std::string("##CPUNE") + uid).c_str())) { editor->AddNode(new x()); ImGui::CloseCurrentPopup(); } }

static int count = 1;

static void ShowNodeMaker(std::string &uid, NodeEditor *editor)
{
	static char data[1000];
	ImGui::InputTextWithHint("##SearchMeshNodes", "Search ...", data, sizeof(data));
	int length = (int)strlen(data);
	ImGui::BeginChild(("##CPUNE" + uid).c_str(), ImVec2(200, 250));
	NODE_MAKER_SHOW(DummyNode, "Dummy");
	NODE_MAKER_SHOW(MeshCoordinatesNode, "Mesh Coordinates");
	NODE_MAKER_SHOW(MaxMeshCoordinatesNode, "Maximum Mesh Coordinates");
	NODE_MAKER_SHOW(TextureCoordinatesNode, "Texture Coordinates");
	NODE_MAKER_SHOW(MinMeshCoordinatesNode, "Minimum Mesh Coordinates");
	NODE_MAKER_SHOW(TimeBasedSeedNode, "Time Based Seed");
	NODE_MAKER_SHOW(PixelateNode, "Pixelate");
	NODE_MAKER_SHOW(TextureNode, "Texture");
	NODE_MAKER_SHOW(HeightmapNode, "Heightmap");
	NODE_MAKER_SHOW(HillNode, "Hill");
	NODE_MAKER_SHOW(AddNode, "Add");
	NODE_MAKER_SHOW(SubNode, "Subtract");
	NODE_MAKER_SHOW(MulNode, "Multiply");
	NODE_MAKER_SHOW(DivNode, "Divide");
	NODE_MAKER_SHOW(AbsNode, "Absolute Value");
	NODE_MAKER_SHOW(SinNode, "Sin");
	NODE_MAKER_SHOW(CosNode, "Cos");
	NODE_MAKER_SHOW(TanNode, "Tan");
	NODE_MAKER_SHOW(SquareNode, "Square");
	NODE_MAKER_SHOW(BlendNode, "Blend");
	NODE_MAKER_SHOW(ClampNode, "Clamp");
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
	NODE_MAKER_SHOW(MinValNode, "MinVal");
	NODE_MAKER_SHOW(VisualizerNode, "Visualizer");
	ImGui::EndChild();
}

CPUNodeEditor::CPUNodeEditor(ApplicationState *as)
	:appState(as)
{
	uid = GenerateId(32);
	name.reserve(1024);
	name = "CPU Node Editor " + std::to_string(count++);
	NodeEditorConfig config;
	config.saveFile = GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "configs" PATH_SEPARATOR "CPUNodeEditorconfigs.terr3d";
	config.makeNodeFunc = [&]()
	{
		ImGuiNodeEditor::Suspend();
		ImGui::OpenPopup("NodeMakerDropped");
		ImGuiNodeEditor::Resume();
	};
	config.insNodeFunc = [&](nlohmann::json data) -> NodeEditorNode*
	{
		NodeEditorNode *node;
		CPUNodeEditorE::CPUNodeType type = (CPUNodeEditorE::CPUNodeType)data["type"];

		switch (type)
		{
			case CPUNodeEditorE::CPUNodeType::Dummy:
				node = new DummyNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Output:
				node = new OutputNode();
				break;

			case CPUNodeEditorE::CPUNodeType::MeshCoordinates:
				node = new MeshCoordinatesNode();
				break;

			case CPUNodeEditorE::CPUNodeType::MaxMeshCoordinates:
				node = new MaxMeshCoordinatesNode();
				break;

			case CPUNodeEditorE::CPUNodeType::MinMeshCoordinates:
				node = new MinMeshCoordinatesNode();
				break;

			case CPUNodeEditorE::CPUNodeType::TextureCoordinates:
				node = new TextureCoordinatesNode();
				break;

			case CPUNodeEditorE::CPUNodeType::TimeBasedSeed:
				node = new TimeBasedSeedNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Duplicate:
				node = new DuplicateNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Add:
				node = new AddNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Sub:
				node = new SubNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Mul:
				node = new MulNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Div:
				node = new DivNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Sin:
				node = new SinNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Cos:
				node = new CosNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Square:
				node = new SquareNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Tan:
				node = new TanNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Abs:
				node = new AbsNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Blend:
				node = new BlendNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Curve:
				node = new CurveNode();
				break;

			case CPUNodeEditorE::CPUNodeType::RandomNumber:
				node = new RandomNumberNode();
				break;

			case CPUNodeEditorE::CPUNodeType::NoisePerlin:
				node = new NoisePerlinNode();
				break;

			case CPUNodeEditorE::CPUNodeType::NoiseCellular:
				node = new NoiseCellularNode();
				break;

			case CPUNodeEditorE::CPUNodeType::NoiseOpenSimplex2:
				node = new NoiseOpenSimplex2Node();
				break;

			case CPUNodeEditorE::CPUNodeType::NoiseOpenSimplex2S:
				node = new NoiseOpenSimplex2SNode();
				break;

			case CPUNodeEditorE::CPUNodeType::NoiseValue:
				node = new NoiseValueNode();
				break;

			case CPUNodeEditorE::CPUNodeType::NoiseValueCubic:
				node = new NoiseValueCubicNode();
				break;

			case CPUNodeEditorE::CPUNodeType::MathFunction:
				node = new MathFunctionNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Texture:
				node = new TextureNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Heightmap:
				node = new HeightmapNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Pixelate:
				node = new PixelateNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Visualizer:
				node = new VisualizerNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Hill:
				node = new HillNode();
				break;

			case CPUNodeEditorE::CPUNodeType::Clamp:
				node = new ClampNode();
				break;

			case CPUNodeEditorE::CPUNodeType::MinVal:
				node = new MinValNode();
				break;

			default:
				node = nullptr;
				Log("Unknown Node Type!");
				break;
		}

		return node;
	};
	editor = new NodeEditor(config);
	editor->name = name;
	editor->SetOutputNode(new OutputNode());
}

CPUNodeEditor::~CPUNodeEditor()
{
	delete editor;
}

nlohmann::json CPUNodeEditor::Save()
{
	nlohmann::json data;
	data["nodeEditor"] = editor->Save();
	data["windowStat"] = windowStat;
	data["enabled"] = enabled;
	data["uiActive"] = uiActive;
	data["uid"] = uid;
	return data;
}

void CPUNodeEditor::Load(nlohmann::json data)
{
	appState->workManager->WaitForFinish();

	editor->Load(data["nodeEditor"]);
	windowStat = data["windowStat"];
	enabled = data["enabled"];
	uiActive = data["uiActive"];
	uid = data["uid"];
}

bool CPUNodeEditor::Update()
{
	bool stateChanged = false;
	if(windowStat)
	{
		ImGui::Begin((name + "##" + uid).c_str(), &windowStat);

		if (ImGui::Button(("Add Node##CPUNE" + uid).c_str())) ImGui::OpenPopup("NodeMakerDropped");
		ImGui::SameLine();
		if (ImGui::Button(("Reset##CPUNE" + uid).c_str())) { editor->Reset(); stateChanged = true; }
		ImGui::SameLine();
		if (ImGui::Button(("Export##CPUNE" + uid).c_str()))
		{
			std::string file = ShowSaveFileDialog("*.terr3d");
			if (file.size() > 3)
			{
				if (file.find(".terr3d") == std::string::npos) file += ".terr3d";
				SaveToFile(file, editor->Save().dump(4));
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(("Import##CPUNE" + uid).c_str()))
		{
			std::string file = ShowOpenFileDialog("*.terr3d");
			if (file.size() > 3)
			{
				bool tmp = false;
				editor->Reset();
				editor->Load(nlohmann::json::parse(ReadShaderSourceFile(file, &tmp)));
				stateChanged = true;
			}
		}
		stateChanged = editor->Render() || stateChanged;

		if (ImGui::IsWindowFocused() && (((IsKeyDown(TERR3D_KEY_RIGHT_SHIFT) || IsKeyDown(TERR3D_KEY_LEFT_SHIFT)) && IsKeyDown(TERR3D_KEY_A)) || IsMouseButtonDown(TERR3D_MOUSE_BUTTON_MIDDLE))) ImGui::OpenPopup("NodeMakerDropped");

		if(ImGui::BeginPopup("NodeMakerDropped"))
		{
			ShowNodeMaker(uid, editor);
			if (ImGui::Button(("Close##CPUNE" + uid).c_str())) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		ImGui::End();
	}

	return stateChanged;
}

float CPUNodeEditor::EvaluateAt(NodeInputParam param)
{
	return editor->outputNode->Evaluate(param, nullptr).value;
}

bool CPUNodeEditor::ShowSetting(int id)
{
	bool stateChanged = false;
	ImGui::InputText(("Name##CPUNE" + uid).c_str(), name.data(), 1024);
	stateChanged |= ImGui::Checkbox(("Enabled##CPUNE" + uid).c_str(), &enabled);

	if (ImGui::Button(("Edit##CPUNE" + uid).c_str()))
	{
		windowStat = true;
	}

	ImGui::Text("UID : %s", uid.data());
	ImGui::Text("Time : %lf ms", time);
	return stateChanged;
}
