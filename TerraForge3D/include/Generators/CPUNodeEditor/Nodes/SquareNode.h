#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class SquareNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	SquareNode();

	float value1, value2;
};

