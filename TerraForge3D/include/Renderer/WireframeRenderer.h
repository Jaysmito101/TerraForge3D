#pragma once


#include "Renderer/RendererBase.h"

class WireframeRenderer : public RendererBase
{
public:
	WireframeRenderer(ApplicationState* appState);
	virtual ~WireframeRenderer();

	virtual void Render(RendererViewport* viewport) override;
	virtual void ShowSettings() override;

protected:
	virtual void ReloadShaders() override;

private:
	bool m_InvertNormals = false;
};
