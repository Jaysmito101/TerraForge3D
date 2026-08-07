#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Inspector/CustomInspector.h"

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::renderer
{
    class RendererLights
    {
    public:
        RendererLights(ApplicationState *appState);
        ~RendererLights();

        void ShowSettings();

        inline exporters::SerializerNode Save() const
        {
            return m_Inspector.SaveState();
        }

        inline bool Load(exporters::SerializerNode data)
        {
            return m_Inspector.LoadState(data);
        }

        inline nlohmann::json GetSettingsSchema() const
        {
            return m_Inspector.BuildSchema();
        }

        inline bool IsSkyLightEnabled() const
        {
            return m_Inspector.Get<bool>("UseSkyLight");
        }

        inline float GetSkyLightIntensity() const
        {
            return m_Inspector.Get<float>("SkyLightIntensity");
        }

        inline std::string GetSunName() const
        {
            return m_Inspector.Get<std::string>("Name");
        }

        inline glm::vec3 GetSunDirection() const
        {
            return m_Inspector.Get<glm::vec3>("Direction");
        }

        inline glm::vec3 GetSunColor() const
        {
            return m_Inspector.Get<glm::vec3>("Color");
        }

        inline float GetSunIntensity() const
        {
            return m_Inspector.Get<float>("Intensity");
        }

    private:
        CustomInspector m_Inspector;
    };

} // namespace tf3d::renderer
