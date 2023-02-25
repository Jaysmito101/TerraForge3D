#pragma once

#include "Base/Base.h"
#include "Renderer/RendererViewport.h"

class ApplicationState;

#define S_RENDERER_MAX_LIGHTS 16

class ShadedRenderer
{
public:
	ShadedRenderer(ApplicationState* appState);
	~ShadedRenderer();

	void Render(RendererViewport* viewport);
	void ShowSettings();

private:
	void ReloadShaders();

private:
	ApplicationState* m_AppState = nullptr;
	Shader* m_Shader = nullptr;
	bool m_InvertNormals = false;
};