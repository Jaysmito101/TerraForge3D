#pragma once


#include "Renderer/RendererBase.h"

class TextureSlotRenderer : public RendererBase
{
public:
	TextureSlotRenderer(ApplicationState* appState);
	virtual ~TextureSlotRenderer();

	virtual void Render(RendererViewport* viewport) override;
	virtual void ShowSettings() override;

private:
	virtual void ReloadShaders() override;

private:
	std::shared_ptr<Model> m_ScreenQuad = nullptr;
	float m_HeightmapMin = -1.0f;
	float m_HeightmapMax = 1.0f;
};