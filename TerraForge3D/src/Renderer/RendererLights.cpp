#include "Renderer/RendererLights.h"
#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

namespace tf3d::renderer
{

    RendererLights::RendererLights(ApplicationState *appState) : m_AppState(appState)
    {
        BuildInspector();
    }

    RendererLights::~RendererLights()
    {
    }

    void RendererLights::BuildInspector()
    {
        if (m_AppState == nullptr || m_AppState->resourceManager == nullptr) {
            TF3D_LOG_ERROR("Cannot load Renderer Lights inspector metadata without a resource manager");
            return;
        }

        const std::string configPath = m_AppState->constants.dataDir + PATH_SEPARATOR + "inspectors" +
                                       PATH_SEPARATOR + "Lights.json";
        bool loaded              = false;
        const std::string source = m_AppState->resourceManager->LoadText(configPath, false, &loaded);
        if (!loaded) {
            TF3D_LOG_ERROR("Could not load Renderer Lights inspector metadata '{}'", configPath);
            return;
        }

        const nlohmann::json config = nlohmann::json::parse(source, nullptr, false);
        if (config.is_discarded()) {
            TF3D_LOG_ERROR("Could not parse Renderer Lights inspector metadata '{}'", configPath);
            return;
        }
        if (!m_Inspector.LoadConfig(config))
            TF3D_LOG_ERROR("Could not load Renderer Lights inspector metadata '{}'", configPath);
    }

    void RendererLights::ShowSettings()
    {
        m_Inspector.Render();
    }

} // namespace tf3d::renderer
