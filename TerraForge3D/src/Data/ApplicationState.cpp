#include "Data/ApplicationState.h"

#include "Base/Logging/Logger.h"

#include <algorithm>
#include <string_view>

namespace
{
    void LoadBoolean(const nlohmann::json &data,
                     const char *key,
                     bool &target)
    {
        const auto value = data.find(key);
        if (value == data.end())
            return;
        if (!value->is_boolean()) {
            TF3D_LOG_WARN("Invalid application window state '{}': expected a boolean", key);
            return;
        }
        target = value->get<bool>();
    }
}

namespace tf3d::data
{

    ApplicationState::ApplicationState()
    {
    }

    ApplicationState::~ApplicationState()
    {
    }

    nlohmann::json ApplicationStateWindows::Save()
    {
        return {
            {"Dashboard", dashboard},
            {"GenerationManager", generationManager},
            {"RendererSettings", rendererSettings},
            {"ExportManager", exportManager},
            {"JobManager", jobManager},
            {"PerformanceMonitor", performanceMonitor},
            {"McpControlPanel", mcpControlPanel},
            {"StyleEditor", styleEditor},
            {"TextureStore", textureStore},
            {"OsLisc", osLisc},
            {"SupportersTribute", supportersTribute},
            {"ViewportVisible", viewportVisible}};
    }

    void ApplicationStateWindows::Load(nlohmann::json data)
    {
        if (!data.is_object()) {
            TF3D_LOG_WARN("Invalid application window state: expected a JSON object");
            return;
        }

        LoadBoolean(data, "Dashboard", dashboard);
        LoadBoolean(data, "GenerationManager", generationManager);
        LoadBoolean(data, "RendererSettings", rendererSettings);
        LoadBoolean(data, "ExportManager", exportManager);
        LoadBoolean(data, "JobManager", jobManager);
        LoadBoolean(data, "PerformanceMonitor", performanceMonitor);
        LoadBoolean(data, "McpControlPanel", mcpControlPanel);
        LoadBoolean(data, "StyleEditor", styleEditor);
        LoadBoolean(data, "TextureStore", textureStore);
        LoadBoolean(data, "OsLisc", osLisc);
        LoadBoolean(data, "SupportersTribute", supportersTribute);

        const auto viewports = data.find("ViewportVisible");
        if (viewports == data.end())
            return;
        if (!viewports->is_array()) {
            TF3D_LOG_WARN("Invalid application window state 'ViewportVisible': expected an array");
            return;
        }

        const std::size_t count = std::min(viewports->size(), viewportVisible.size());
        for (std::size_t index = 0; index < count; ++index) {
            if (!viewports->at(index).is_boolean()) {
                TF3D_LOG_WARN(
                    "Invalid application window state 'ViewportVisible[{}]': expected a boolean",
                    index);
                continue;
            }
            viewportVisible[index] = viewports->at(index).get<bool>();
        }
    }

    nlohmann::json ApplicationStateStates::Save()
    {
        nlohmann::json data;
        return data;
    }

    void ApplicationStateStates::Load(nlohmann::json data)
    {
    }

    nlohmann::json ApplicationStateGlobals::Save()
    {
        nlohmann::json data;
        return data;
    }

    void ApplicationStateGlobals::Load(nlohmann::json data)
    {
    }

} // namespace tf3d::data
