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
#include <thread>
#include <atomic>
#include <Texture2D.h>
#include <GLFW/glfw3.h>
#include <Application.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <MathUtils.h>


static Editor editorM;
static float zoom = 1.0f;
static bool flag = false;
static bool showHMap = false;
static bool showNodeMaker = false;
static std::atomic<bool> isRefilling = false;
static std::atomic<bool> allowRefilling = true;
static bool drop_make_node = false;
static int drop_midway_start = -1;
static FloatValueI* outputNode;
static int* resolution;
static int currRes;
static float* heightMapData;
static float* heightMapDataMT;
static unsigned char* heightMap;
static bool autoUpdate = false;
static Texture2D* hMap;

char* strcasestr(const char* str, const char* pattern) {
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
	while (isRefilling);
	float* heightMapDataC;

	if (heightMap)
		delete[] heightMap;
	if (hMap)
		delete hMap;
	if (heightMapData)
		delete[] heightMapData;
	if (heightMapDataMT)
		delete[] heightMapDataMT;
	hMap = new Texture2D(currRes, currRes);
	heightMapData = new float[res * res];
	memset(heightMapData, 0, res * res);
	heightMapDataMT = new float[res * res];
	memset(heightMapDataMT, 0, res * res);
	heightMap = new unsigned char[res * res * 3];
	memset(heightMap, 0, res * res * 3);
}

static void UpdateHeightMapMTImpl() {
	float t = 0;
	for (int i = 0; i < currRes; i++) {
		for (int j = 0; j < currRes; j++) {
			heightMapDataMT[i * currRes + j] = outputNode->Evaluate(i, j);
			unsigned char d = (unsigned char)((heightMapDataMT[i * currRes + j] * 0.5f + 0.5f) * 256);
			heightMap[i * currRes * 3 + j * 3 + 0] = d;
			heightMap[i * currRes * 3 + j * 3 + 1] = d;
			heightMap[i * currRes * 3 + j * 3 + 2] = d;
		}
	}
	isRefilling = false;
}

static void UpdateHeightMapMT() {
	if (!isRefilling) {

		if (heightMapDataMT) {
			hMap->SetData(heightMap, currRes * currRes * 3);
			float* tmp = heightMapData;
			heightMapData = heightMapDataMT;
			heightMapDataMT = tmp;
			memset(tmp, 0, currRes * currRes);
		}

		isRefilling = true;
		std::thread t(UpdateHeightMapMTImpl);
		t.detach();
	}
	else {
		// For future
	}
}

static void UpdateHeightMap() {
	float t = 0;
	for (int i = 0; i < currRes; i++) {
		for (int j = 0; j < currRes; j++) {
			heightMapData[i * currRes + j] = outputNode->Evaluate(i, j);
			unsigned char d = (unsigned char)((heightMapData[i * currRes + j] * 0.5f + 0.5f) * 256);
			heightMap[i * currRes * 3 + j * 3 + 0] = d;
			heightMap[i * currRes * 3 + j * 3 + 1] = d;
			heightMap[i * currRes * 3 + j * 3 + 2] = d;
		}
	}
	hMap->SetData(heightMap, currRes * currRes * 3);
	if (*resolution != currRes)
		return;
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
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::FloatNodeO) {
		FloatValueO* nd = new FloatValueO();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::MeshCoord) {
		MeshCoordNode* nd = new MeshCoordNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Add) {
		AdderNode* nd = new AdderNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Sub) {
		SubtractNode* nd = new SubtractNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Div) {
		DivideNode* nd = new DivideNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Mul) {
		MultiplyNode* nd = new MultiplyNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Sin) {
		SinNode* nd = new SinNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Cos) {
		CosNode* nd = new CosNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Abs) {
		AbsNode* nd = new AbsNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Square) {
		SquareNode* nd = new SquareNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Sqrt) {
		SqrtNode* nd = new SqrtNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Clamp) {
		ClampNode* nd = new ClampNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::ScriptF) {
		ScriptFloatNode* nd = new ScriptFloatNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Mix) {
		MixNode* nd = new MixNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::TextureSampler2D) {
		TextureSamplerNode* nd = new TextureSamplerNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}

	if (nodedata["type"] == NodeType::Random) {
		RandomNode* nd = new RandomNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}
	if (nodedata["type"] == NodeType::Time) {
		TimeNode* nd = new TimeNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}
	if (nodedata["type"] == NodeType::Perlin) {
		PerlinNode* nd = new PerlinNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}
	if (nodedata["type"] == NodeType::Cellular) {
		CellularNode* nd = new CellularNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}
	if (nodedata["type"] == NodeType::Generator) {
		GeneratorNode* nd = new GeneratorNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}
	if (nodedata["type"] == NodeType::Condition) {
		ConditionNode* nd = new ConditionNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
		return nd;
	}
	if (nodedata["type"] == NodeType::Duplicate) {
		DuplicateNode* nd = new DuplicateNode();
		nd->Load(nodedata);
		nd->data = NodeData(resolution);
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
	for (Node* n : editor.nodes) {
		if (n->id == outputNode->id)
			continue;
		delete n;
	}
	editor.nodes.clear();
	editor.pins.clear();
	ImNodes::EditorContextSet(editor.context);
	ImNodes::LoadEditorStateFromIniString(editor.context, std::string(data["context"]).c_str(), std::string(data["context"]).size());
	SetBaseId(data["lID"]);
	for (nlohmann::json node : data["nodes"]) {
		editor.nodes.push_back(MakeNode(node));
	}
	for (Node* n : editor.nodes) {
		std::vector<void*> np = n->GetPins();
		for (void* pn : np) {
			editor.pins.push_back((Pin*)pn);
		}
	}
	editor.pins.push_back(&(outputNode)->inputPin);
	editor.links.clear();
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
	/*
	for (nlohmann::json nodePos : data["nodePositions"]) {
		ImNodes::SetNodeEditorSpacePos(nodePos["id"], ImVec2(nodePos["x"], nodePos["y"]));
	}
	*/
}

void SetElevationNodeEditorSaveData(nlohmann::json data) {
	try {
		LoadEditorData(editorM, data);
		autoUpdate = data["updateNode"];
	}
	catch (...) {
		std::cout << ("Failed to Load Node Editor Data.\n");
	}
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

static void ShowNodesMaker(Pin* start_drop, char* searchString, int searchStringLength) {
	if (searchStringLength == 0 || strcasestr("Mesh Coordinates", searchString)) {
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
	}

	if (searchStringLength == 0 || strcasestr("Float Value", searchString)) {
		if (ImGui::Button("Float Value"))
		{
			const int node_id = ++editorM.current_id;
			editorM.nodes.push_back(new FloatValueO("Float Value"));
			editorM.pins.push_back(&((FloatValueO*)editorM.nodes.back())->outputPin);
			((FloatValueO*)editorM.nodes.back())->Setup();
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos());
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Add", searchString)) {
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
	}

	if (searchStringLength == 0 || strcasestr("Subtract", searchString)) {
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
	}

	if (searchStringLength == 0 || strcasestr("Multiply", searchString)) {
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
	}

	if (searchStringLength == 0 || strcasestr("Divide", searchString)) {
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
	}

	if (searchStringLength == 0 || strcasestr("Square", searchString)) {
		if (ImGui::Button("Square"))
		{
			editorM.nodes.push_back(new SquareNode());
			editorM.pins.push_back(&((SquareNode*)editorM.nodes.back())->outputPin);
			editorM.pins.push_back(&((SquareNode*)editorM.nodes.back())->inputPin);
			((SquareNode*)editorM.nodes.back())->Setup();
			if (start_drop) {
				MakeLink(start_drop, &((SquareNode*)editorM.nodes.back())->inputPin);
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Square Root", searchString)) {
		if (ImGui::Button("Square Root"))
		{
			editorM.nodes.push_back(new SqrtNode());
			editorM.pins.push_back(&((SqrtNode*)editorM.nodes.back())->outputPin);
			editorM.pins.push_back(&((SqrtNode*)editorM.nodes.back())->inputPin);
			((SqrtNode*)editorM.nodes.back())->Setup();
			if (start_drop) {
				MakeLink(start_drop, &((SqrtNode*)editorM.nodes.back())->inputPin);
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Absolute Value", searchString)) {
		if (ImGui::Button("Absolute Value"))
		{
			editorM.nodes.push_back(new AbsNode());
			std::vector<void*> pins = ((AbsNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((AbsNode*)editorM.nodes.back())->Setup();
			if (start_drop) {
				MakeLink(start_drop, &((AbsNode*)editorM.nodes.back())->inputPin);
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Clamp", searchString)) {
		if (ImGui::Button("Clamp"))
		{
			editorM.nodes.push_back(new ClampNode());
			std::vector<void*> pins = ((ClampNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((ClampNode*)editorM.nodes.back())->Setup();
			if (start_drop) {
				MakeLink(start_drop, &((ClampNode*)editorM.nodes.back())->inputPinV);
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Mix", searchString)) {
		if (ImGui::Button("Mix"))
		{
			editorM.nodes.push_back(new MixNode());
			std::vector<void*> pins = ((MixNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((MixNode*)editorM.nodes.back())->Setup();
			if (start_drop) {
				MakeLink(start_drop, &((MixNode*)editorM.nodes.back())->inputPinV);
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Texture Sampler", searchString)) {
		if (ImGui::Button("Texture Sampler"))
		{
			editorM.nodes.push_back(new TextureSamplerNode());
			std::vector<void*> pins = ((TextureSamplerNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((TextureSamplerNode*)editorM.nodes.back())->Setup();
			((TextureSamplerNode*)editorM.nodes.back())->data = NodeData(resolution);
			if (start_drop) {
				// Nothing to do here as it takes no input!
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Duplicate", searchString)) {
		if (ImGui::Button("Duplicate"))
		{
			editorM.nodes.push_back(new DuplicateNode());
			std::vector<void*> pins = ((DuplicateNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((DuplicateNode*)editorM.nodes.back())->Setup();
			((DuplicateNode*)editorM.nodes.back())->data = NodeData(resolution);
			if (start_drop) {
				MakeLink(start_drop, &((DuplicateNode*)editorM.nodes.back())->inputPin);
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Script", searchString)) {
		if (ImGui::Button("Script"))
		{
			editorM.nodes.push_back(new ScriptFloatNode());
			std::vector<void*> pins = ((ScriptFloatNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((ScriptFloatNode*)editorM.nodes.back())->data = NodeData(resolution);
			((ScriptFloatNode*)editorM.nodes.back())->Setup();
			if (start_drop) {
				MakeLink(start_drop, &((ScriptFloatNode*)editorM.nodes.back())->inputPinV);
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Random", searchString)) {
		if (ImGui::Button("Random"))
		{
			editorM.nodes.push_back(new RandomNode());
			std::vector<void*> pins = ((RandomNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((RandomNode*)editorM.nodes.back())->Setup();
			((RandomNode*)editorM.nodes.back())->data = NodeData(resolution);
			if (start_drop) {
				MakeLink(start_drop, &((RandomNode*)editorM.nodes.back())->inputPin);
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Simplex Noise", searchString)) {
		if (ImGui::Button("Simplex Noise"))
		{
			editorM.nodes.push_back(new PerlinNode());
			std::vector<void*> pins = ((PerlinNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((PerlinNode*)editorM.nodes.back())->Setup();
			((PerlinNode*)editorM.nodes.back())->data = NodeData(resolution);
			if (start_drop) {
				MakeLink(start_drop, &((PerlinNode*)editorM.nodes.back())->inputPinT);
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}


	if (searchStringLength == 0 || strcasestr("Celllar Noise", searchString)) {
		if (ImGui::Button("Celllar Noise"))
		{
			editorM.nodes.push_back(new CellularNode());
			std::vector<void*> pins = ((CellularNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((CellularNode*)editorM.nodes.back())->Setup();
			((CellularNode*)editorM.nodes.back())->data = NodeData(resolution);
			if (start_drop) {
				MakeLink(start_drop, &((CellularNode*)editorM.nodes.back())->inputPinT);
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Generator", searchString)) {
		if (ImGui::Button("Generator"))
		{
			editorM.nodes.push_back(new GeneratorNode());
			std::vector<void*> pins = ((GeneratorNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((GeneratorNode*)editorM.nodes.back())->Setup();
			((GeneratorNode*)editorM.nodes.back())->data = NodeData(resolution);
			if (start_drop) {
				// Nothing to do in here!
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Time", searchString)) {
		if (ImGui::Button("Time"))
		{
			editorM.nodes.push_back(new TimeNode());
			std::vector<void*> pins = ((TimeNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((TimeNode*)editorM.nodes.back())->Setup();
			((TimeNode*)editorM.nodes.back())->data = NodeData(resolution);
			if (start_drop) {
				// Nothing to do as this has no input pins!
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Condition", searchString)) {
		if (ImGui::Button("Condition"))
		{
			editorM.nodes.push_back(new ConditionNode());
			std::vector<void*> pins = ((ConditionNode*)editorM.nodes.back())->GetPins();
			for (void* p : pins)
				editorM.pins.push_back((Pin*)p);
			((ConditionNode*)editorM.nodes.back())->Setup();
			((ConditionNode*)editorM.nodes.back())->data = NodeData(resolution);
			if (start_drop) {
				// Nothing to do as this has no input pins!
			}
			ImNodes::SetNodeScreenSpacePos(editorM.nodes.back()->id, ImGui::GetMousePos() - ImVec2(40, 40));
			ImGui::CloseCurrentPopup();
		}
	}

	if (searchStringLength == 0 || strcasestr("Sin", searchString)) {
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
	}

	if (searchStringLength == 0 || strcasestr("Cos", searchString)) {
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
	}
}

static void UpdateNodeDuplacation() {
	for (Node* node : editorM.nodes) {
		if (ImNodes::IsNodeSelected(node->id)) {
			if (node->id == outputNode->id)
				return;
		}
	}
}

nlohmann::json GetElevationNodeEditorSaveData() {
	nlohmann::json data = editorM.Save(outputNode);
	data["updateNode"] = autoUpdate;
	return data;
}

static void ShowNodeMaker(Pin* start_drop) {
	static char searchString[1024];
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_PopupBg]);
	ImGui::InputTextWithHint("##ENodeSearch", "Serch", searchString, 1024);
	ImGui::BeginChild("NodeMakerPopup", ImVec2(200, 200));
	ImGui::Separator();
	int searchStringLength = strlen(searchString);
	ShowNodesMaker(start_drop, searchString, searchStringLength);
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
			if (glfwGetKey(Application::Get()->GetWindow()->GetNativeWindow(), GLFW_KEY_DELETE) && (glfwGetKey(Application::Get()->GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_SHIFT) || glfwGetKey(Application::Get()->GetWindow()->GetNativeWindow(), GLFW_KEY_LEFT_CONTROL))) {
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
	}
	else {
		if (ImGui::Button("Show Viewport on Output Node")) {
			if (outputNode) {
				outputNode->outputTex = GetViewportFramebufferColorTextureId();
				showHMap = false;
			}
		}
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

	{
		UpdateNodeDeletion();
		UpdateNodeDuplacation();


		int start_id;
		if (ImNodes::IsLinkDropped(&start_id, false)) {
			if (drop_make_node) {
				drop_midway_start = start_id;
			}
			ImGui::OpenPopup("NodeMakerDropped");
		}

		if (ImGui::BeginPopup("NodeMakerDropped"))
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
}

void ShowElevationNodeEditor(bool* pOpen)
{
	if (currRes != *resolution) {
		currRes = *resolution;
		SetUpHeightMap(currRes);
		UpdateHeightMap();
	}
	if (autoUpdate) {
		UpdateHeightMapMT();
	}
	ImGui::Begin("Elevation Node Editor", pOpen);

	ShowEditor("editor1", editorM);

	ImGui::End();
}

float GetElevation(float x, float y) {
	return heightMapData[(int)x * currRes + (int)y];
}

void ElevationNodeEditorTick() {

}