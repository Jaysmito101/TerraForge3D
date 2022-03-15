#pragma once

#include "Shading/ShaderNodeEditor.h"
#include "Shading/ShaderTextureManager.h"
#include "Base/Texture2D.h"

class ShaderTextureNode : public SNENode
{
public:
	ShaderTextureNode(GLSLHandler* handler, ShaderTextureManager* textureManager);
	~ShaderTextureNode();

	virtual void OnEvaluate(GLSLFunction* function, GLSLLine* line) override;
	virtual void Load(nlohmann::json data) override;
	virtual nlohmann::json Save() override;
	virtual void OnRender() override;
	virtual void UpdateShaders() override;

	Texture2D* texture = nullptr;
	ShaderTextureManager* textureManager = nullptr;
	float scale = 1.0f;
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	float rotation = 0.0f;
	uint32_t zCoord = 0;
};