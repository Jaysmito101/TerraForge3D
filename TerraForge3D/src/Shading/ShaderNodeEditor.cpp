#include "Shading/ShaderNodeEditor.h"

// ------------------ SNEPin ------------------

SNEPin::SNEPin(NodeEditorPinType type, SNEPinType var)
	: NodeEditorPin(type)
{
	SetType(var);
}

SNEPin::~SNEPin()
{
}

SNEPinType SNEPin::GetType()
{
	return static_cast<SNEPinType>(userData);
}

void SNEPin::SetType(SNEPinType type)
{
	userData = type;

	switch(type)
	{
		case SNEPinType_Float:
			color = ImColor(FLOAT_PIN_COLOR);
			break;

		case SNEPinType_Float2:
			color = ImColor(FLOAT2_PIN_COLOR);
			break;

		case SNEPinType_Float3:
			color = ImColor(FLOAT3_PIN_COLOR);
			break;

		case SNEPinType_Bool:
			color = ImColor(BOOL_PIN_COLOR);
			break;
	}
}

// ------------------ SNENode ------------------

SNENode::SNENode(GLSLHandler *handler)
	:handler(handler)
{
}

SNENode::~SNENode()
{
}

NodeOutput SNENode::Evaluate(NodeInputParam input, NodeEditorPin *pin)
{
	GLSLFunction *function = static_cast<GLSLFunction *>(input.userData1);
	GLSLLine *line = static_cast<GLSLLine *>(input.userData2);
	OnEvaluate(function, line);
	return NodeOutput({0.0f});
}

bool SNENode::OnLink(NodeEditorPin *pin, NodeEditorLink *link)
{
	SNEPin *selfPin = static_cast<SNEPin *>(pin);
	SNEPin *otherPin = nullptr;

	if(selfPin->type == NodeEditorPinType::Input)
	{
		otherPin = static_cast<SNEPin *>(link->to);
	}

	else
	{
		otherPin = static_cast<SNEPin *>(link->from);
	}

	return (selfPin->GetType() == otherPin->GetType());
}