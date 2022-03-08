#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class PixelateNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	PixelateNode();

	float pixelSize;
};

