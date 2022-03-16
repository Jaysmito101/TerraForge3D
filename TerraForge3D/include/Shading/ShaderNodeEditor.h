#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include "Shading/SharedMemoryManager.h"
#include "Shading/GLSLHandler.h"


// NOTE : These colors were suggested by GitHub Copilot

#define FLOAT_PIN_COLOR 23, 245, 235
#define FLOAT2_PIN_COLOR 223, 155, 121
#define FLOAT3_PIN_COLOR 154, 231, 123
#define BOOL_PIN_COLOR 150, 191, 235

#define SHADER_OUTPUT_NODE_COLOR 152, 54, 12
#define SHADER_VALUE_NODE_COLOR 144, 124, 255
#define SHADER_TEXTURE_NODE_COLOR 145, 32, 123
#define SHADER_MATERIAL_NODE_COLOR 251, 123, 144

#define STR(x) std::to_string(x)
#define VAR(x) std::string(x) + STR(id)
#define SDATA(x) std::string("data[") + STR(dataBlobOffset) + "].d[" + STR(x) + "]"

enum SNEPinType
{
	SNEPinType_Float = 0,
	SNEPinType_Float2,
	SNEPinType_Float3,
	SNEPinType_Bool
};

static NodeInputParam GetParams(GLSLFunction *function, GLSLLine *line)
{
	NodeInputParam param;
	param.userData1 = static_cast<void *>(function);
	param.userData2 = static_cast<void *>(line);
	return param;
}


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
	SNENode(GLSLHandler *handler);
	~SNENode();

	NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual bool OnLink(NodeEditorPin *pin, NodeEditorLink *link);

	virtual void UpdateShaders() = 0;
	virtual void OnRender() = 0;
	virtual void OnEvaluate(GLSLFunction *function, GLSLLine *line) = 0;
	virtual void Load(nlohmann::json data) = 0;
	virtual nlohmann::json Save() = 0;

public:
	SharedMemoryItem *sharedData = nullptr;
	int dataBlobOffset = 0;
	GLSLHandler *handler;
};

