#pragma once

#include "Renderer/RendererBase.h"

#define S_RENDERER_MAX_LIGHTS 16

class ShadedRenderer : public RendererBase
{
public:
	ShadedRenderer(ApplicationState* appState);
	virtual ~ShadedRenderer();

	virtual void Render(RendererViewport* viewport) override;
	virtual void ShowSettings() override;

private:
	virtual void ReloadShaders() override;

private:
	bool m_InvertNormals = false;
};