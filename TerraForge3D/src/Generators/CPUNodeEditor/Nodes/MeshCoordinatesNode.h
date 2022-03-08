#pragma once

#include "Base/NodeEditor/NodeEditor.h"

class MeshCoordinatesNode : public NodeEditorNode
{
public:
	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);
	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();
	MeshCoordinatesNode();
};

