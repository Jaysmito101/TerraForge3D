#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include "Shading/GLSLHandler.h"

#define FLOAT_PIN_COLOR 23, 145, 235
#define FLOAT2_PIN_COLOR 123, 25, 21
#define FLOAT3_PIN_COLOR 54, 31, 123
#define BOOL_PIN_COLOR 21, 91, 200

#define SHADER_OUTPUT_NODE_COLOR 152, 54, 12

#define STR(x) std::to_string(x)

enum SNEPinType
{
	SNEPinType_Float = 0,
	SNEPinType_Float2,
	SNEPinType_Float3,
	SNEPinType_Bool
};

class SNEPin : public NodeEditorPin
{
public:
	SNEPin(NodeEditorPinType type = NodeEditorPinType::Input, SNEPinType var = SNEPinType::SNEPinType_Float);

	~SNEPin();

	SNEPinType GetType();
	void SetType(SNEPinType type);
};

class SNENode : public NodeEditorNode
{
public:
	SNENode(GLSLHandler* handler);
	~SNENode();

	NodeOutput Evaluate(NodeInputParam input, NodeEditorPin* pin);
	
	virtual bool OnLink(NodeEditorPin* pin, NodeEditorLink* link);

	virtual void OnRender() = 0;
	virtual void OnEvaluate(GLSLFunction* function, GLSLLine* line) = 0;
	virtual void Load(nlohmann::json data) = 0;
	virtual nlohmann::json Save() = 0;

public:
	int dataBlobOffset = 0;
	GLSLHandler* handler;
};

