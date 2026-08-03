#pragma once

#include "Renderer/RendererBase.h"
#include "Renderer/BrushSettings.h"

#define OBJECT_RENDERER_MAX_LIGHTS 16

struct BiomeCustomBaseShapeDrawSettings;


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
