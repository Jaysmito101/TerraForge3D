#pragma once

#include "Renderer/RendererBase.h"
#include "Renderer/BrushSettings.h"

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
	std::shared_ptr<Shader> m_PostProcessShader;
	uint32_t m_PostProcessVao = 0;
	DrawBrushSettings* m_DrawBrushSettings = nullptr;
};
