#pragma once

struct ApplicationState;

class MainMenu
{
public:
	MainMenu(ApplicationState* appState);
	~MainMenu();

	void ShowMainMenu();

	void ShowFileMenu();
	void ShowOptionsMenu();
	void ShowWindowsMenu();
	void ShowHelpMenu();

private:
	ApplicationState* appState;
};