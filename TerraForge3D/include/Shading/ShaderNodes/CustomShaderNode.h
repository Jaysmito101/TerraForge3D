#pragma once

#include "Shading/ShaderNodeEditor.h"

class CustomShaderNode : public SNENode
{
public:
	CustomShaderNode(GLSLHandler *handler, std::string shader);
	~CustomShaderNode();

	virtual void OnEvaluate(GLSLFunction *function, GLSLLine *line) override;
	virtual void Load(nlohmann::json data) override;
	virtual nlohmann::json Save() override;
	virtual void OnRender() override;
	virtual void UpdateShaders() override;

public:
	float fData[32];
	bool bData[32];
	std::string code = "";
	std::string shader = "";
	nlohmann::json meta;
	GLSLFunction *func;
	std::vector<std::pair<std::string, std::string>> params;
	std::vector<std::pair<std::string, std::string>> sharedDataTemplate;
};