#pragma once

#include "Renderer/RendererViewport.h"
#include "Renderer/ObjectRenderer.h"
#include "Renderer/HeightmapRenderer.h"
#include "Renderer/TextureSlotRenderer.h"
#include "Renderer/WireframeRenderer.h"

#include "Renderer/RendererLights.h"
#include "Renderer/RendererSky.h"
#include "Renderer/PlanarShadowCache.h"
#include "Renderer/TerrainSelfShadow.h"
#include "Renderer/HeightfieldAmbientCache.h"

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
	inline TerrainSelfShadow* GetTerrainSelfShadow() { return m_TerrainSelfShadow.get(); }
	inline PlanarShadowCache* GetPlanarShadowCache() { return m_PlanarShadowCache.get(); }
	inline HeightfieldAmbientCache* GetHeightfieldAmbientCache() { return m_HeightfieldAmbientCache.get(); }

	inline ObjectRenderer* GetObjectRenderer() { return static_cast<ObjectRenderer*>(m_ObjectRenderer.get()); }
	inline HeightmapRenderer* GetHeightmapRenderer() { return static_cast<HeightmapRenderer*>(m_HeightmapRenderer.get()); }
	inline TextureSlotRenderer* GetTextureSlotRenderer() { return static_cast<TextureSlotRenderer*>(m_TextureSlotRenderer.get()); }
	inline WireframeRenderer* GetWireframeRenderer() { return static_cast<WireframeRenderer*>(m_WireframeRenderer.get()); }



private:
	ApplicationState* m_AppState = nullptr;
	bool m_IsWindowVisible = true;
	// std::shared_ptr<ObjectRenderer> m_ObjectRenderer;
	// std::shared_ptr<HeightmapRenderer> m_HeightmapRenderer;
	// std::shared_ptr<TextureSlotRenderer> m_TextureSlotRenderer;
	// std::shared_ptr<WireframeRenderer> m_WireframeRenderer;
	std::shared_ptr<RendererBase> m_ObjectRenderer, m_HeightmapRenderer, m_TextureSlotRenderer, m_WireframeRenderer;

	std::shared_ptr<RendererLights> m_RendererLights;
	std::shared_ptr<RendererSky> m_RendererSky;
	std::shared_ptr<TerrainSelfShadow> m_TerrainSelfShadow;
	std::shared_ptr<PlanarShadowCache> m_PlanarShadowCache;
	std::shared_ptr<HeightfieldAmbientCache> m_HeightfieldAmbientCache;
	bool m_EnableAmbientAo = true;
	float m_AmbientAoRadiusFactor = 0.12f;
};

