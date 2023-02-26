#pragma once

#include "Renderer/RendererViewport.h"
#include "Renderer/ObjectRenderer.h"
#include "Renderer/HeightmapRenderer.h"
#include "Renderer/ShadedRenderer.h"
#include "Renderer/TextureSlotRenderer.h"
#include "Renderer/WireframeRenderer.h"

#include "Renderer/RendererLights.h"
#include "Renderer/RendererSky.h"

class ApplicationState;

class RendererManager
{
public:
	RendererManager(ApplicationState* appState);
	~RendererManager();

	void Render(RendererViewport* viewport);
	void ShowSettings();
	inline bool IsWindowVisible() { return this->m_IsWindowVisible; }
	inline bool* IsWindowVisiblePtr() { return &this->m_IsWindowVisible; }

	inline RendererLights* GetRendererLights() { return this->m_RendererLights; }
	inline RendererSky* GetSkyRenderer() { return this->m_RendererSky; }

private:
	ApplicationState* m_AppState = nullptr;
	bool m_IsWindowVisible = true;
	ObjectRenderer* m_ObjectRenderer = nullptr;
	HeightmapRenderer* m_HeightmapRenderer = nullptr;
	ShadedRenderer* m_ShadedRenderer = nullptr;
	TextureSlotRenderer* m_TextureSlotRenderer = nullptr;
	WireframeRenderer* m_WireframeRenderer = nullptr;

	RendererLights* m_RendererLights = nullptr;
	RendererSky* m_RendererSky = nullptr;
};

