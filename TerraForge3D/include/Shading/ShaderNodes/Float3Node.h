#pragma once

#include "Shading/ShaderNodeEditor.h"

class Float3Node : public SNENode
{
public:
	Float3Node(GLSLHandler *handler);
	~Float3Node();

	virtual void OnEvaluate(GLSLFunction *function, GLSLLine *line) override;
	virtual void Load(nlohmann::json data) override;
	virtual nlohmann::json Save() override;
	virtual void OnRender() override;
	virtual void UpdateShaders() override;

	float x, y, z;
};