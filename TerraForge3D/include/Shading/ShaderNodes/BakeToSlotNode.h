#pragma once

#include "Shading/ShaderNodeEditor.h"

class BakeToSlotNode : public SNENode
{
public:
	BakeToSlotNode(GLSLHandler *handler);
	~BakeToSlotNode();

	virtual void OnEvaluate(GLSLFunction *function, GLSLLine *line) override;
	virtual void Load(nlohmann::json data) override;
	virtual nlohmann::json Save() override;
	virtual void OnRender() override;
	virtual void UpdateShaders() override;

public:
    int slot = 1;
};