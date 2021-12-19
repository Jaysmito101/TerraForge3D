#pragma once
#include "json.hpp"
#include "imgui-node-editor/imgui_node_editor.h"

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <iterator>
#include <functional>
#include <mutex>

#define MAKE_IMGUI_ID(x) ("##" + std::to_string(x)).c_str()
#define MAKE_IMGUI_LABEL(x, y) (y + std::string("##") + std::to_string(x)).c_str()

namespace ImGuiNodeEditor = ax::NodeEditor;

enum NodeEditorPinType
{
	Output = 0,
	Input,
	PinTypeCount
};

struct NodeOutput 
{
	float value;
};

struct NodeInputParam 
{
	float x;
	float y;
	float z;

	float texX;
	float texY;

	float minX;
	float minY;
	float minZ;

	float maxX;
	float maxY;
	float maxZ;

	NodeInputParam();
	NodeInputParam(float* pos, float* texCoord, float* minPos, float* maxPos);
};


int GenerateUID();
void SeUIDSeed(int seed);

class NodeEditorNode;
class NodeEditorPin;

struct NodeEditorConfig 
{
	std::string saveFile;
	std::function<void(void)> updateFunc;
	std::function<void(void)> makeNodeFunc;
	std::function<NodeEditorNode*(nlohmann::json)> insNodeFunc;

	NodeEditorConfig(std::string saveFile = "NodeEditor.terr3d");
};

class NodeEditorLink 
{
public:
	int id;
	ImGuiNodeEditor::LinkId _id;
	NodeEditorPin* from;
	NodeEditorPin* to;
	ImVec4 color = ImVec4(1, 1, 1, 1);
	float thickness = 1.0f;

	nlohmann::json Save();
	void Load(nlohmann::json data);

	NodeEditorLink(int id = GenerateUID());
};

class NodeEditorPin
{
public:
	int id;
	ImGuiNodeEditor::PinId _id;
	NodeEditorLink* link;
	NodeEditorPin* other;
	NodeEditorNode* parent;
	NodeEditorPinType type;
	std::mutex mutex;
	ImU32 color = ImColor(94, 95, 191);
	char userData[128];

	virtual nlohmann::json Save();
	virtual void Load(nlohmann::json data);
	virtual void Begin();
	virtual void End();
	bool ValidateLink(NodeEditorLink* link);
	void Link(NodeEditorLink* link);
	bool IsLinked();
	void Unlink();
	void Render();
	NodeOutput Evaluate(NodeInputParam input);


	NodeEditorPin(NodeEditorPinType type = NodeEditorPinType::Input, int id = GenerateUID());
	~NodeEditorPin();
};

class NodeEditorNode
{
public:
	int id;
	ImGuiNodeEditor::NodeId _id;
	std::vector<NodeEditorPin*> outputPins;
	std::vector<NodeEditorPin*> inputPins;
	char userData[128];
	std::string name;
	ImU32 headerColor = ImColor(59, 29, 209);
	std::mutex m;

	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin* pin) = 0;
	virtual std::vector<NodeEditorPin*> GetPins();
	virtual bool OnLink(NodeEditorPin* pin, NodeEditorLink* link);
	virtual void OnDelete();

	nlohmann::json SaveInternal();
	void LoadInternal(nlohmann::json data);
	virtual void Load(nlohmann::json data) = 0;
	virtual nlohmann::json Save() = 0;
	virtual void OnRender() = 0;

	void Render();
	void Setup();
	void DrawHeader(std::string text);

	NodeEditorNode(int id =  GenerateUID());
	~NodeEditorNode();
};


class NodeEditor 
{
public:
	ImGuiNodeEditor::EditorContext* context;
	std::string name = "Node Editor";
	NodeEditorConfig config;
	std::unordered_map<uintptr_t,  NodeEditorLink*> links;
	std::unordered_map<uintptr_t,  NodeEditorNode*> nodes;
	std::unordered_map<uintptr_t, NodeEditorPin*> pins;
	NodeEditorNode* outputNode = nullptr;
	std::mutex mutex;

	nlohmann::json Save();
	void Load(nlohmann::json data);
	void Render();
	void AddNode(NodeEditorNode* node);
	void DeleteNode(NodeEditorNode* node);
	void DeleteLink(NodeEditorLink* link);
	void Reset();
	void SetOutputNode(NodeEditorNode* node);

	NodeEditor(NodeEditorConfig config = NodeEditorConfig());
	~NodeEditor();

private:
	ImGuiNodeEditor::NodeId lastNodeId;
};