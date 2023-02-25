#pragma once

#include "Base/Base.h"
#include "Renderer/RendererViewport.h"

class ApplicationState;

class TextureSlotRenderer
{
public:
	TextureSlotRenderer(ApplicationState* appState);
	~TextureSlotRenderer();

	void Render(RendererViewport* viewport);
	void ShowSettings();

private:
	void ReloadShaders();

private:
	ApplicationState* m_AppState = nullptr;
	Shader* m_Shader = nullptr;
	Model* m_ScreenQuad = nullptr;
	float m_HeightmapMin = -1.0f;
	float m_HeightmapMax = 1.0f;
};