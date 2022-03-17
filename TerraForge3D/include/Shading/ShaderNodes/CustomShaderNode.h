#pragma once

#include "Shading/ShaderNodeEditor.h"
struct SharedDataRep {
	std::string text;
	std::string type;
	std::string alias;

	SharedDataRep(std::string t, std::string b, std::string c)
	:text(t), type(b), alias(c) {}

	SharedDataRep(){}
};

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
	int paramCount = 0;
	bool useArrayParams = false;
	nlohmann::json meta;
	GLSLFunction *func;
	std::vector<std::pair<std::string, std::string>> params;
	std::vector<SharedDataRep> sharedDataTemplate;
};
