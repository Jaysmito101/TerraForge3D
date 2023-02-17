#pragma once

#include "Base/NodeEditor/NodeEditor.h"

namespace mu
{
class Parser;
}

class MathFunctionNode : public NodeEditorNode
{
public:
	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);
	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();
	MathFunctionNode();

	char inputExpression[1024 * 4];
	mu::Parser *parser;
	double x, y, z;
	float factor;
	bool compiled;
	int mathInputWidth;
	std::vector<std::pair<std::string, double>> vars;
	char varname[4096];
	int tid = GenerateUID(); // temp
};

