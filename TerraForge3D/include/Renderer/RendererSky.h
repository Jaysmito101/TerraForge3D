#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Misc/CustomInspector.h"
#include "Renderer/RendererViewport.h"

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::renderer
{

    class RendererSky
    {
    public:
        RendererSky(ApplicationState *appState);
        ~RendererSky();

        void ShowSettings();
        void Render(RendererViewport *viewport);

        inline exporters::SerializerNode Save() const
        {
            auto state = m_Inspector.SaveState();
            state->Set("SkyReady", m_IsSkyReady);
            return state;
        }

        bool Load(exporters::SerializerNode data);
        bool LoadSkyMap(const std::string &path);
        void ReloadShaders();

        inline nlohmann::json GetSettingsSchema() const
        {
            return m_Inspector.BuildSchema();
        }

        inline int32_t GetIrradianceMap()
        {
            return m_IrradianceMapTextureID;
        }
        inline int32_t GetSpecularMap()
        {
            return m_SpecularMapTextureID;
        }
        inline int32_t GetBrdfLut()
        {
            return m_BrdfLutTextureID;
        }
        inline int32_t GetSkyboxMap()
        {
            return m_SkyboxTextureID;
        }
        inline bool IsSkyReady()
        {
            return m_IsSkyReady;
        }

    private:
        bool LoadSkyboxTexture(const std::string &path);

    private:
        data::ApplicationState *m_AppState = nullptr;
        misc::CustomInspector m_Inspector;
        std::shared_ptr<base::ComputeShader> m_EquirectToCube = nullptr;
        std::shared_ptr<base::ComputeShader> m_SpecularMap    = nullptr;
        std::shared_ptr<base::ComputeShader> m_IrradianceMap  = nullptr;
        std::shared_ptr<base::ComputeShader> m_BrdfLut        = nullptr;
        std::shared_ptr<base::GraphicsShader> m_SkyboxShader  = nullptr;
        base::Model *m_SkyboxModel                            = nullptr;
        bool m_IsSkyReady                                     = false;
        uint32_t m_SkyboxTextureID                            = -1;
        uint32_t m_IrradianceMapTextureID                     = -1;
        uint32_t m_SpecularMapTextureID                       = -1;
        uint32_t m_BrdfLutTextureID                           = -1;
    };

} // namespace tf3d::renderer
