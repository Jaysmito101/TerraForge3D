#pragma once

#ifdef TF3D_ENABLE_MCP

#include <string>

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::ui
{

    class McpControlPanel
    {
    public:
        explicit McpControlPanel(ApplicationState *appState);
        ~McpControlPanel() = default;

        void ShowSettings();
        bool *IsWindowVisiblePtr();

    private:
        ApplicationState *m_AppState = nullptr;
        bool m_RuntimeEnabled        = false;
        bool m_RestartRequired       = false;
        std::string m_LastError;
        std::string m_CopyMessage;
    };
} // namespace tf3d::ui

#endif
