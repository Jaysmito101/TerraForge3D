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
	inline bool IsWindowVisible() { return m_IsWindowVisible; }
	inline bool* IsWindowVisiblePtr() { return &m_IsWindowVisible; }

	inline RendererLights* GetRendererLights() { return m_RendererLights.get(); }
	inline RendererSky* GetSkyRenderer() { return m_RendererSky.get(); }

	inline ObjectRenderer* GetObjectRenderer() { return static_cast<ObjectRenderer*>(m_ObjectRenderer.get()); }
	inline HeightmapRenderer* GetHeightmapRenderer() { return static_cast<HeightmapRenderer*>(m_HeightmapRenderer.get()); }
	inline ShadedRenderer* GetShadedRenderer() { return static_cast<ShadedRenderer*>(m_ShadedRenderer.get()); }
	inline TextureSlotRenderer* GetTextureSlotRenderer() { return static_cast<TextureSlotRenderer*>(m_TextureSlotRenderer.get()); }
	inline WireframeRenderer* GetWireframeRenderer() { return static_cast<WireframeRenderer*>(m_WireframeRenderer.get()); }



private:
	ApplicationState* m_AppState = nullptr;
	bool m_IsWindowVisible = true;
	// std::shared_ptr<ObjectRenderer> m_ObjectRenderer;
	// std::shared_ptr<HeightmapRenderer> m_HeightmapRenderer;
	// std::shared_ptr<ShadedRenderer> m_ShadedRenderer;
	// std::shared_ptr<TextureSlotRenderer> m_TextureSlotRenderer;
	// std::shared_ptr<WireframeRenderer> m_WireframeRenderer;
	std::shared_ptr<RendererBase> m_ObjectRenderer, m_HeightmapRenderer, m_ShadedRenderer, m_TextureSlotRenderer, m_WireframeRenderer;

	std::shared_ptr<RendererLights> m_RendererLights;
	std::shared_ptr<RendererSky> m_RendererSky;
};

