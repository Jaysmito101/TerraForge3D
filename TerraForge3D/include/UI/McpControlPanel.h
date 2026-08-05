#pragma once

#ifdef TF3D_ENABLE_MCP

#include <string>

class ApplicationState;

class McpControlPanel
{
public:
    explicit McpControlPanel(ApplicationState *appState);
    ~McpControlPanel() = default;

    void ShowSettings();
    bool *IsWindowVisiblePtr()
    {
        return &m_IsWindowVisible;
    }

private:
    ApplicationState *m_AppState = nullptr;
    bool m_IsWindowVisible       = true;
    bool m_RuntimeEnabled        = false;
    bool m_RestartRequired       = false;
    std::string m_LastError;
    std::string m_CopyMessage;
};

#endif
