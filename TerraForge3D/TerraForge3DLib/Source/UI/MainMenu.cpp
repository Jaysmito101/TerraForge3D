#include "UI/MainMenu.hpp"

#include "TerraForge3D.hpp"

namespace TerraForge3D
{

	

	MainMenu::MainMenu(ApplicationState* appState)
	{
		this->appState = appState;
		menuManager.SetMainMenu(true);
		Setup();
	}

	MainMenu::~MainMenu()
	{
	}

	void MainMenu::Show()
	{
		menuManager.Show();
	}

	void MainMenu::Setup()
	{
		menuManager.Register("File/Close", [this](UI::MenuItem*) {appState->core.app->Close(); });
	}

}