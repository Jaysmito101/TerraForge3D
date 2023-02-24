#pragma once

#include "Base/Base.h"
#include "Renderer/RendererViewport.h"

class ApplicationState;

class WireframeRenderer
{
public:
	WireframeRenderer(ApplicationState* appState);
	~WireframeRenderer();

	void Render(RendererViewport* viewport);
	void ShowSettings();

private:
	void ReloadShaders();

private:
	ApplicationState* m_AppState = nullptr;
	Shader* m_Shader = nullptr;
	bool m_InvertNormals = false;
};
