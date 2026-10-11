#include "UI/ApiControlPanel.h"

#ifdef TF3D_ENABLE_API

#include "API/ApiServer.h"
#include "Data/ApplicationState.h"
#include "Data/ConfigManager.h"

#include "imgui/imgui.h"

namespace tf3d::ui
{

    ApiControlPanel::ApiControlPanel(tf3d::data::ApplicationState *appState)
        : m_AppState(appState), m_RuntimeEnabled(appState != nullptr && appState->apiEnabled)
    {
    }

    bool *ApiControlPanel::IsWindowVisiblePtr()
    {
        return m_AppState != nullptr ? &m_AppState->windows.apiControlPanel : nullptr;
    }

    void ApiControlPanel::ShowSettings()
    {
        if (m_AppState == nullptr || !m_AppState->windows.apiControlPanel) {
            return;
        }
        m_CopyMessage.clear();

        ImGui::Begin("API Server", &m_AppState->windows.apiControlPanel);

        api::ApiServerStats stats;
        if (m_AppState->apiServer != nullptr) {
            stats = m_AppState->apiServer->GetStats();
        }

        const char *runtimeStatus = "Unavailable";
        if (stats.running) {
            runtimeStatus = "Running";
        } else if (m_RuntimeEnabled) {
            runtimeStatus = "Stopped";
        } else {
            runtimeStatus = "Disabled";
        }

        ImGui::Text("Status: %s", runtimeStatus);
        ImGui::Text("Address: %s:%d", stats.host.c_str(), stats.port);
        ImGui::Text("API: %s", stats.endpoint.c_str());
        ImGui::Text("Requests: %llu", static_cast<unsigned long long>(stats.requestCount));
        ImGui::SameLine();
        ImGui::Text("Active: %llu", static_cast<unsigned long long>(stats.activeRequests));
        if (ImGui::Button("Copy API URL")) {
            ImGui::SetClipboardText(stats.endpoint.c_str());
            m_CopyMessage = "API URL copied.";
        }
        ImGui::SameLine();
        const std::string apiOrigin     = "http://" + stats.host + ":" + std::to_string(stats.port);
        const std::string playgroundUrl = apiOrigin + "/api/docs";
        if (ImGui::Button("Copy playground URL")) {
            ImGui::SetClipboardText(playgroundUrl.c_str());
            m_CopyMessage = "Playground URL copied.";
        }
        if (ImGui::Button("Copy agent prompt")) {
            const std::string skillGuideUrl = apiOrigin + "/api/skill.md";
            const std::string prompt =
                "To get started with TerraForge3D, fetch the API skill guide at " + skillGuideUrl + ".";
            ImGui::SetClipboardText(prompt.c_str());
            m_CopyMessage = "Agent prompt copied.";
        }
        if (!m_CopyMessage.empty()) {
            ImGui::TextDisabled("%s", m_CopyMessage.c_str());
        }

        ImGui::Separator();

        bool enabled = m_AppState->apiEnabled;
        if (ImGui::Checkbox("Enable API server on next restart", &enabled)) {
            if (m_AppState->configManager != nullptr &&
                m_AppState->configManager->SetBool("api", "enabled", enabled)) {
                m_AppState->apiEnabled = enabled;
                m_RestartRequired      = enabled != m_RuntimeEnabled;
                m_LastError.clear();
            } else {
                m_LastError = "Could not save the API setting to the user config.";
            }
        }

        if (m_RestartRequired) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                               "Restart TerraForge3D for this change to take effect.");
        }
        if (!m_RuntimeEnabled) {
            ImGui::TextDisabled("The API is disabled for this process. Enable it above and restart to start the server.");
        }
        if (!m_LastError.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", m_LastError.c_str());
        }

        if (ImGui::Button("Clear request log") && m_AppState->apiServer != nullptr) {
            m_AppState->apiServer->ClearRequestLog();
        }

        ImGui::Separator();
        const bool childVisible = ImGui::BeginChild("##ApiRequestLog", ImVec2(0.0f, 0.0f), true);
        if (childVisible) {
            const bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
            if (stats.requestLog.empty()) {
                ImGui::TextDisabled("No API requests received yet.");
            } else {
                for (std::size_t index = 0; index < stats.requestLog.size(); ++index) {
                    const auto &request = stats.requestLog[index];
                    ImGui::PushID(static_cast<int>(index));
                    ImGui::TextDisabled("[%s]", request.timestamp.c_str());
                    ImGui::SameLine();
                    ImGui::Text("%s %s (%d)", request.method.c_str(), request.path.c_str(), request.status);
                    if (ImGui::SmallButton("Copy body")) {
                        ImGui::SetClipboardText(request.bodyPreview.c_str());
                        m_CopyMessage = "Request body copied.";
                    }
                    ImGui::Separator();
                    ImGui::PopID();
                }
            }
            if (wasAtBottom) {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }

} // namespace tf3d::ui
#endif
