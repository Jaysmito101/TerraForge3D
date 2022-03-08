#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class HillNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	HillNode();

	float pos[3], height, radius, p;
};

