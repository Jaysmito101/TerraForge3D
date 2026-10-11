#pragma once

#ifdef TF3D_ENABLE_API

#include <string>

namespace tf3d::data
{
    class ApplicationState;
}

namespace tf3d::ui
{

    class ApiControlPanel
    {
    public:
        explicit ApiControlPanel(tf3d::data::ApplicationState *appState);
        ~ApiControlPanel() = default;

        void ShowSettings();
        bool *IsWindowVisiblePtr();

    private:
        tf3d::data::ApplicationState *m_AppState = nullptr;
        bool m_RuntimeEnabled                    = false;
        bool m_RestartRequired                   = false;
        std::string m_LastError;
        std::string m_CopyMessage;
    };
} // namespace tf3d::ui

#endif
