#pragma once

#include "Shading/ShaderNodeEditor.h"

class FloatNode : public SNENode
{
public:
	FloatNode(GLSLHandler *handler);
	~FloatNode();

	virtual void OnEvaluate(GLSLFunction *function, GLSLLine *line) override;
	virtual void Load(nlohmann::json data) override;
	virtual nlohmann::json Save() override;
	virtual void OnRender() override;
	virtual void UpdateShaders() override;

	float x, y, z;
};
