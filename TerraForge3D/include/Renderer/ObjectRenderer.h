#pragma once

#include "Renderer/RendererBase.h"

#define OBJECT_RENDERER_MAX_LIGHTS 16

struct BiomeCustomBaseShapeDrawSettings;


struct DrawBrushSettings
{
	float m_BrushSize = 0.2f;
	float m_BrushStrength = 0.5f;
	float m_BrushFalloff = 0.5f;
	float m_BrushPositionX = 0.0f;
	float m_BrushPositionY = 0.0f;
	float m_BrushRotation = 0.0f;
	int m_BrushMode = 0;

	int32_t m_MaskTexture = -1;
	glm::vec3 m_MaskColor = glm::vec3(1.0f);
};

class ObjectRenderer : public RendererBase 
{
public:
	ObjectRenderer(ApplicationState* appState);
	virtual ~ObjectRenderer();

	virtual void Render(RendererViewport* viewport) override;
	virtual void ShowSettings() override;
	inline void SetCustomBaseShapeDrawSettings(DrawBrushSettings* settings) { m_DrawBrushSettings = settings; }

private:
	virtual void ReloadShaders() override;

private:
	bool m_InvertNormals = false;
	std::shared_ptr<ShaderStorageBuffer> m_SharedMemoryBuffer;
	DrawBrushSettings* m_DrawBrushSettings = nullptr;
};