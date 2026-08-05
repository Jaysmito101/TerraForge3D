#include "UI/McpControlPanel.h"

#ifdef TF3D_ENABLE_MCP

#include "Data/ApplicationState.h"
#include "Data/ConfigManager.h"
#include "MCP/TerraForgeMcpServer.h"

#include "imgui/imgui.h"
#include "nlohmann/json.hpp"

#include <algorithm>

namespace
{
    using UiJson = nlohmann::ordered_json;

    struct McpCommandDisplay {
        std::string title;
        std::string detail;
    };

    std::string TruncateForDisplay(const std::string &value, std::size_t maxLength)
    {
        if (value.size() <= maxLength)
            return value;
        return value.substr(0, maxLength) + "...";
    }

    std::string GetString(const UiJson &object, const char *key)
    {
        if (!object.is_object() || !object.contains(key) || !object[key].is_string())
            return {};
        return object[key].get<std::string>();
    }

    UiJson VisibleParameters(const UiJson &parameters)
    {
        UiJson visible = parameters;
        if (visible.is_object())
            visible.erase("_meta");
        return visible;
    }

    McpCommandDisplay SummarizeCommand(const McpCommandLogEntry &command)
    {
        McpCommandDisplay display{command.method, {}};
        UiJson parameters;
        try {
            parameters = UiJson::parse(command.parameters);
        } catch (...) {
            display.detail = "Request details are available with Copy details.";
            return display;
        }

        if (command.method == "initialize") {
            const UiJson clientInfo         = parameters.value("clientInfo", UiJson::object());
            const std::string clientName    = GetString(clientInfo, "name");
            const std::string clientVersion = GetString(clientInfo, "version");
            display.title                   = "Initialize";
            display.detail                  = "Client: " + (clientName.empty() ? "Unknown" : clientName);
            if (!clientVersion.empty())
                display.detail += " " + clientVersion;
            return display;
        }

        if (command.method == "notifications/initialized") {
            display.title = "Session ready";
            return display;
        }

        if (command.method == "tools/call") {
            display.title = "Call tool: " + GetString(parameters, "name");
            if (display.title == "Call tool: ")
                display.title = "Call tool";
            if (parameters.is_object() && parameters.contains("arguments") && !parameters["arguments"].empty()) {
                display.detail = "Arguments: " +
                                 TruncateForDisplay(VisibleParameters(parameters["arguments"]).dump(), 280);
            }
            return display;
        }

        if (command.method == "resources/read") {
            display.title         = "Read resource";
            const std::string uri = GetString(parameters, "uri");
            if (!uri.empty())
                display.detail = uri;
            return display;
        }

        if (command.method == "tools/list")
            display.title = "List tools";
        else if (command.method == "resources/list")
            display.title = "List resources";
        else if (command.method == "resources/templates/list")
            display.title = "List resource templates";
        else
            display.title = command.method;

        const UiJson visible = VisibleParameters(parameters);
        if (!visible.is_null() && !visible.empty())
            display.detail = TruncateForDisplay(visible.dump(), 280);
        return display;
    }
} // namespace

McpControlPanel::McpControlPanel(ApplicationState *appState)
    : m_AppState(appState), m_RuntimeEnabled(appState != nullptr && appState->mcpEnabled)
{
}

void McpControlPanel::ShowSettings()
{
    if (!m_IsWindowVisible)
        return;
    m_CopyMessage.clear();

    ImGui::Begin("MCP Server", &m_IsWindowVisible);

    McpServerStats stats;
    if (m_AppState && m_AppState->mcpServer)
        stats = m_AppState->mcpServer->GetStats();

    const char *runtimeStatus = "Unavailable";
    if (stats.running)
        runtimeStatus = "Running";
    else if (m_RuntimeEnabled)
        runtimeStatus = "Stopped";
    else
        runtimeStatus = "Disabled";

    ImGui::Text("Status: %s", runtimeStatus);
    ImGui::Text("IP: %s", stats.host.c_str());
    ImGui::SameLine();
    ImGui::Text("Port: %d", stats.port);
    ImGui::Text("Endpoint: %s", stats.endpoint.c_str());
    ImGui::Text("Commands: %llu", static_cast<unsigned long long>(stats.commandCount));
    ImGui::SameLine();
    ImGui::Text("Active sessions: %llu", static_cast<unsigned long long>(stats.activeSessions));
    if (ImGui::Button("Copy endpoint")) {
        ImGui::SetClipboardText(stats.endpoint.c_str());
        m_CopyMessage = "Endpoint copied.";
    }
    ImGui::SameLine();
    if (!m_CopyMessage.empty())
        ImGui::TextDisabled("%s", m_CopyMessage.c_str());

    ImGui::Separator();

    bool enabled = m_AppState && m_AppState->mcpEnabled;
    if (ImGui::Checkbox("Enable MCP server on next restart", &enabled)) {
        if (m_AppState && m_AppState->configManager &&
            m_AppState->configManager->SetBool("mcp", "enabled", enabled)) {
            m_AppState->mcpEnabled = enabled;
            m_RestartRequired      = enabled != m_RuntimeEnabled;
            m_LastError.clear();
        } else {
            m_LastError = "Could not save the MCP setting to the user config.";
        }
    }

    if (m_RestartRequired) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                           "Restart TerraForge3D for this change to take effect.");
    }

    if (!m_RuntimeEnabled) {
        ImGui::TextDisabled("MCP is disabled for this process. Enable it above and restart to start the server.");
    }
    if (!m_LastError.empty())
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", m_LastError.c_str());

    if (ImGui::Button("Clear command log") && m_AppState && m_AppState->mcpServer)
        m_AppState->mcpServer->ClearCommandLog();

    ImGui::Separator();
    const bool childVisible = ImGui::BeginChild("##McpCommandLog", ImVec2(0.0f, 0.0f), true);
    if (childVisible) {
        const bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
        if (stats.commandLog.empty()) {
            ImGui::TextDisabled("No MCP commands received yet.");
        } else {
            for (std::size_t index = 0; index < stats.commandLog.size(); ++index) {
                const auto &command             = stats.commandLog[index];
                const McpCommandDisplay display = SummarizeCommand(command);
                ImGui::PushID(static_cast<int>(index));
                ImGui::TextDisabled("[%s]", command.timestamp.c_str());
                ImGui::SameLine();
                ImGui::TextUnformatted(display.title.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("Copy details")) {
                    const std::string copyText = command.requestJson.empty()
                                                     ? command.method + "\n" + command.parameters
                                                     : command.requestJson;
                    ImGui::SetClipboardText(copyText.c_str());
                    m_CopyMessage = "Request details copied.";
                }
                if (!display.detail.empty()) {
                    ImGui::TextDisabled("%s", TruncateForDisplay(display.detail, 320).c_str());
                }
                ImGui::Separator();
                ImGui::PopID();
            }
        }

        if (wasAtBottom)
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}

#endif
