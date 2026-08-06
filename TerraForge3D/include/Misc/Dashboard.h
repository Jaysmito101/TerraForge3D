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

        bool IsWindowVisible() const;
        bool *IsWindowVisiblePtr();
        void SetWindowVisible(bool visible);

    private:
        void CalculateTileSizeAndOffset();
        void ShowChooseBaseModelPopup();

    private:
        ApplicationState *m_AppState = nullptr;
        bool m_ForceUpdate           = false;
    };

} // namespace tf3d::misc
using tf3d::misc::Dashboard;
