#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class MulNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	MulNode();

	float value1, value2;
};
