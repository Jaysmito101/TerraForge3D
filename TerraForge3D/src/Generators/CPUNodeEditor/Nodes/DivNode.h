#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class DivNode : public NodeEditorNode {
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin* pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	DivNode();

	float value1, value2;
};
