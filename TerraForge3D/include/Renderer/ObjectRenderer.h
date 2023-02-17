#pragma once

#include "Base/Base.h"
#include "Renderer/RendererViewport.h"

class ApplicationState;

#define OBJECT_RENDERER_MAX_LIGHTS 16

class ObjectRenderer
{
public:
	ObjectRenderer(ApplicationState* appState);
	~ObjectRenderer();

	void Render(RendererViewport* viewport);
	void ShowSettings();

private:
	void ReloadShaders();

private:
	ApplicationState* m_AppState = nullptr;
	Shader* m_Shader = nullptr;
	bool m_InvertNormals = false;
};