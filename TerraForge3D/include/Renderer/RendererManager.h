#pragma once

#include "Exporters/Serializer.h"
#include "Inspector/CustomInspector.h"
#include "Renderer/HeightmapRenderer.h"
#include "Renderer/ObjectRenderer.h"
#include "Renderer/RendererViewport.h"
#include "Renderer/SeaRenderer.h"
#include "Renderer/TextureSlotRenderer.h"
#include "Renderer/WireframeRenderer.h"

#include "Renderer/HeightfieldAmbientCache.h"
#include "Renderer/HeightfieldGICache.h"
#include "Renderer/PlanarShadowCache.h"
#include "Renderer/RendererLights.h"
#include "Renderer/RendererSky.h"
#include "Renderer/TerrainSelfShadow.h"

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::renderer
{

    class RendererManager
    {
    public:
        RendererManager(ApplicationState *appState);
        ~RendererManager();

        void Update();
        void Render(RendererViewport *viewport);
        void ShowSettings();
        exporters::SerializerNode SaveTerrainSettings() const;
        bool LoadTerrainSettings(exporters::SerializerNode data);
        inline nlohmann::json GetTerrainSettingsSchema() const
        {
            return m_TerrainInspector.BuildSchema();
        }
        bool IsWindowVisible() const;
        bool *IsWindowVisiblePtr();

        inline RendererLights *GetRendererLights()
        {
            return m_RendererLights.get();
        }
        inline RendererSky *GetSkyRenderer()
        {
            return m_RendererSky.get();
        }
        inline TerrainSelfShadow *GetTerrainSelfShadow()
        {
            return m_TerrainSelfShadow.get();
        }
        inline PlanarShadowCache *GetPlanarShadowCache()
        {
            return m_PlanarShadowCache.get();
        }
        inline float GetPlanarShadowSoftness() const
        {
            return m_TerrainInspector.Get<float>("BasePlaneShadowSoftness", 4.0f);
        }
        inline HeightfieldAmbientCache *GetHeightfieldAmbientCache()
        {
            return m_HeightfieldAmbientCache.get();
        }
        inline HeightfieldGICache *GetHeightfieldGICache()
        {
            return m_HeightfieldGICache.get();
        }
        inline SeaRenderer *GetSeaRenderer()
        {
            return m_SeaRenderer.get();
        }

        inline ObjectRenderer *GetObjectRenderer()
        {
            return static_cast<ObjectRenderer *>(m_ObjectRenderer.get());
        }
        inline HeightmapRenderer *GetHeightmapRenderer()
        {
            return static_cast<HeightmapRenderer *>(m_HeightmapRenderer.get());
        }
        inline TextureSlotRenderer *GetTextureSlotRenderer()
        {
            return static_cast<TextureSlotRenderer *>(m_TextureSlotRenderer.get());
        }
        inline WireframeRenderer *GetWireframeRenderer()
        {
            return static_cast<WireframeRenderer *>(m_WireframeRenderer.get());
        }

    private:
        void UpdateTerrainSelfShadowCache();
        void UpdatePlanarShadowCache();
        void UpdateHeightfieldAmbientCache();
        void UpdateHeightfieldGICache();
        void UpdateTerrainPicking(RendererViewport *viewport);

        ApplicationState *m_AppState = nullptr;
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
        std::shared_ptr<HeightfieldGICache> m_HeightfieldGICache;
        std::shared_ptr<SeaRenderer> m_SeaRenderer;
        CustomInspector m_TerrainInspector;
    };

} // namespace tf3d::renderer
