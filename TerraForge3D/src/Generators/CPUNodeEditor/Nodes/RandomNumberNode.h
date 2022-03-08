#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>
#include <random>

class RandomNumberNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	RandomNumberNode();

	int seed, min, max;
	std::mt19937 engine;
};


