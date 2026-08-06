#pragma once

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::misc
{

    class Dashboard
    {
    public:
        Dashboard(ApplicationState *appState);
        ~Dashboard();

        void Update();
        void ShowSettings();

        inline bool IsWindowVisible() const
        {
            return m_IsWindowVisible;
        }
        inline bool *IsWindowVisiblePtr()
        {
            return &m_IsWindowVisible;
        } // For ImGui::Checkbox
        inline void SetWindowVisible(bool visible)
        {
            m_IsWindowVisible = visible;
        }

    private:
        void CalculateTileSizeAndOffset();
        void ShowChooseBaseModelPopup();

    private:
        ApplicationState *m_AppState = nullptr;
        bool m_IsWindowVisible       = true;
        bool m_ForceUpdate           = false;
    };

} // namespace tf3d::misc
using tf3d::misc::Dashboard;