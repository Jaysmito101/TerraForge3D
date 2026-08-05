#pragma once

#include "Renderer/BrushSettings.h"
#include "Renderer/RendererBase.h"

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
    std::shared_ptr<ShaderStorageBuffer> m_SharedMemoryBuffer;
    std::shared_ptr<Shader> m_PostProcessShader;
    uint32_t m_PostProcessVao              = 0;
    DrawBrushSettings *m_DrawBrushSettings = nullptr;
};
