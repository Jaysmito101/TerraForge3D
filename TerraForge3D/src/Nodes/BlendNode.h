#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class BlendNode : public NodeEditorNode {
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin* pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	BlendNode();

	float value1, value2, factor;
};


