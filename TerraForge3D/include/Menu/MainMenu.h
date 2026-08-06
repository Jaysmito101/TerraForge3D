#pragma once

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::ui
{
class MainMenu
{
public:
    MainMenu(ApplicationState *appState);
    ~MainMenu();

    void ShowMainMenu();

    void ShowFileMenu();
    void ShowOptionsMenu();
    void ShowWindowsMenu();
    void ShowHelpMenu();

private:
    ApplicationState *appState;
};
}