#pragma once

#include "Shading/ShaderNodeEditor.h"

class PBRMaterialNode : public SNENode
{
public:
	PBRMaterialNode(GLSLHandler *handler);
	~PBRMaterialNode();

	virtual void OnEvaluate(GLSLFunction *function, GLSLLine *line) override;
	virtual void Load(nlohmann::json data) override;
	virtual nlohmann::json Save() override;
	virtual void OnRender() override;
	virtual void UpdateShaders() override;

public:
	bool gammaCorrection = true;
	bool hdrTonemapping = true;
	bool invertNormals = false;
};