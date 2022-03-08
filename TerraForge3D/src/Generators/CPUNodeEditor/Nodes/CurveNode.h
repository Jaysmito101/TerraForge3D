#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class CurveNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	CurveNode();

	std::vector<ImVec2> curve;
	int maxPoints;
	int axis;
};

