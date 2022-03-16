#pragma once

#include "Shading/ShaderNodeEditor.h"

class ShaderOutputNode : public SNENode
{
public:
	ShaderOutputNode(GLSLHandler *handler);
	~ShaderOutputNode();

	virtual void OnEvaluate(GLSLFunction *function, GLSLLine *line) override;
	virtual void Load(nlohmann::json data) override;
	virtual nlohmann::json Save() override;
	virtual void OnRender() override;
	virtual void UpdateShaders() override;

	float color[4];
};