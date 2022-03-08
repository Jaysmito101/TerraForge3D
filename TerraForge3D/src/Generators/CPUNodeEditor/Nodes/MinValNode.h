#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class MinValNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	MinValNode();

	float outputf, outputr, inputf, thresholdf;
};

