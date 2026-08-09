#pragma once

#include "Exporters/Serializer.h"
#include "Inspector/CustomInspector.h"
#include "Renderer/RendererBase.h"

#include <cstdint>

namespace tf3d::renderer
{

    class SeaRenderer : public RendererBase
    {
    public:
        explicit SeaRenderer(ApplicationState *appState);
        ~SeaRenderer() override;

        void Render(RendererViewport *viewport) override;
        void ShowSettings() override;

        inline exporters::SerializerNode Save() const
        {
            return m_Inspector.SaveState();
        }

        inline bool Load(exporters::SerializerNode data)
        {
            return m_Inspector.LoadState(data);
        }

        inline bool ResetSettings()
        {
            if (!m_Inspector.IsResetEnabled())
                return false;
            m_Inspector.Reset();
            return true;
        }

        inline nlohmann::json GetSettingsSchema() const
        {
            return m_Inspector.BuildSchema();
        }

        inline bool IsEnabled() const
        {
            return m_Inspector.Root().Get<bool>("Enabled");
        }

    public:
        void ReloadShaders() override;

    private:
        void BindUniforms(RendererViewport *viewport, float terrainWorldSize,
                          float terrainHeightOffset, float seaWorldHeight,
                          const glm::vec2 &surfaceMinimumXZ, const glm::vec2 &surfaceWorldSize);

        CustomInspector m_Inspector;
        uint32_t m_Vao      = 0;
        float m_ElapsedTime = 0.0f;
    };

} // namespace tf3d::renderer
