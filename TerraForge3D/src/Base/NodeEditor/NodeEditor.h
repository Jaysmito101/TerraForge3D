#pragma once
#include "json.hpp"
#include "imgui-node-editor/imgui_node_editor.h"

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <iterator>
#include <functional>

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
};

struct NodeEditorConfig 
{
	std::string saveFile;

	NodeEditorConfig(std::string saveFile = "NodeEditor.terr3d");
};

int GenerateUID();
void SeUIDSeed(int seed);

class NodeEditorNode;
class NodeEditorPin;

class NodeEditorLink 
{
public:
	int id;
	ImGuiNodeEditor::LinkId _id;
	NodeEditorPin* from;
	NodeEditorPin* to;
	NodeEditorPin* other;
	ImVec4 color = ImVec4(1, 1, 1, 1);
	float thickness = 1.0f;

	nlohmann::json Save();

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
	ImColor color = ImColor(94, 95, 191);
	void* userData;

	virtual nlohmann::json Save();
	virtual void Load(nlohmann::json data);
	virtual void Begin();
	virtual void End();
	bool ValidateLink(NodeEditorLink* link);
	void Link(NodeEditorLink* link);
	bool IsLinked();
	void Unlink();
	void Render();

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
	void* userData;
	ImColor headerColor = ImColor(59, 29, 209);

	virtual NodeOutput Evaluate(NodeInputParam input) = 0;

	virtual std::vector<NodeEditorPin*> GetPins();
	virtual bool OnLink(NodeEditorPin* pin, NodeEditorLink* link);
	virtual void OnDelete();

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
	std::function<void(void)> updateFunc;
	std::function<NodeEditorNode*(NodeEditorPin*)> makeNodeFunc;

	nlohmann::json Save();
	void Load(nlohmann::json data);
	void Render();
	void AddNode(NodeEditorNode* node);
	void DeleteNode(NodeEditorNode* node);

	NodeEditor(NodeEditorConfig config = NodeEditorConfig());
	~NodeEditor();
};