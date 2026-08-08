#pragma once

#include "Renderer/BrushSettings.h"
#include "Renderer/RendererBase.h"

namespace tf3d::renderer
{
    struct BiomeCustomBaseShapeDrawSettings;

    class ObjectRenderer : public RendererBase
    {
    public:
        ObjectRenderer(ApplicationState *appState);
        virtual ~ObjectRenderer();

        virtual void Render(RendererViewport *viewport) override;
        virtual void ShowSettings() override;
        inline void SetCustomBaseShapeDrawSettings(DrawBrushSettings *settings)
        {
            m_DrawBrushSettings = settings;
        }

    private:
        virtual void ReloadShaders() override;

    private:
        bool m_InvertNormals         = false;
        bool m_ViewNormals           = false;
        bool m_ViewSlope             = false;
        bool m_ViewTerrainSelfShadow = false;
        bool m_ViewTerrainAmbient    = false;
        bool m_ViewTerrainBentNormal = false;
        std::optional<base::GraphicsShader> m_PostProcessShader;
        uint32_t m_PostProcessVao              = 0;
        DrawBrushSettings *m_DrawBrushSettings = nullptr;
    };
} // namespace tf3d::renderer
