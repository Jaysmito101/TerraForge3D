#pragma once

#include "Base/NodeEditor/NodeEditor.h"

namespace mu {
	class Parser;
}

class MathFunctionNode : public NodeEditorNode {
public:
	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin* pin);
	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();
	MathFunctionNode();

	char inputExpression[1024 * 4];
	mu::Parser* parser;
	double x, y, z;
	bool compiled;
	int mathInputWidth;
};

