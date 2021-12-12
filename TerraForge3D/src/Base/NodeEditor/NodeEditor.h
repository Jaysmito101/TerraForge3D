#pragma once
#include "json.hpp"
#include "imgui-node-editor/imgui_node_editor.h"

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <iterator>

namespace ImGuiNodeEditor = ax::NodeEditor;

enum NodeEditorPinType
{
	Unknown = 0,
	Output,
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

int GenerateUID();
void SeUIDSeed(int seed);

class NodeEditorNode;
class NodeEditorPin;

class NodeEditorLink 
{
public:
	int id;
	ImGuiNodeEditor::LinkId linkId;
	NodeEditorPin* from;
	NodeEditorPin* to;

	nlohmann::json Save();

	NodeEditorLink(int id = GenerateUID());
};

class NodeEditorPin
{
public:
	int id;
	ImGuiNodeEditor::PinId pinId;
	NodeEditorLink* link;
	NodeEditorNode* parent;
	NodeEditorPinType type;
	void* userData;

	virtual nlohmann::json Save() = 0;
	virtual void Load(nlohmann::json data) = 0;
	virtual void Begin();
	virtual void End();
	bool ValidateLink(NodeEditorLink* link);
	void Link(NodeEditorLink* link);

	NodeEditorPin(NodeEditorPinType type = NodeEditorPinType::Input, int id = GenerateUID());
	~NodeEditorPin();
};

class NodeEditorNode
{
public:
	int id;
	std::vector<NodeEditorPin*> outputPins;
	std::vector<NodeEditorPin*> inputPins;
	void* userData;

	virtual  NodeOutput Evaluate(NodeInputParam input) = 0;

	virtual std::vector<NodeEditorPin*> GetPins();
	virtual bool OnLink(NodeEditorPin* pin, NodeEditorLink* link) = 0;

	virtual void Load(nlohmann::json data) = 0;
	virtual nlohmann::json Save() = 0;
	virtual void OnRender() = 0;

	void Render();

	NodeEditorNode();
	~NodeEditorNode();
};


class NodeEditor 
{
public:
	ImGuiNodeEditor::EditorContext* context;
	std::string name = "Node Editor";
	std::unordered_map<int, NodeEditorLink*> links;
	std::unordered_map<int, NodeEditorNode*> nodes;
	std::unordered_map<int, NodeEditorPin*> pins;

	nlohmann::json Save();
	void Load(nlohmann::json data);
	void Render();

	NodeEditor();
	~NodeEditor();
};