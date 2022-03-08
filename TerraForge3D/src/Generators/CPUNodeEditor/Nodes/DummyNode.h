#pragma once

#include "Base/NodeEditor/NodeEditor.h"

class DummyNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);
	virtual bool OnLink(NodeEditorPin *pin, NodeEditorLink *link);
	virtual void OnDelete();

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	DummyNode();
};
