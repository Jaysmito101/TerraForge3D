#pragma once
#include "Base/Core/Core.hpp"
#include "UI/Menu.hpp"

namespace TerraForge3D
{
	class ApplicationState;

	class MainMenu
	{
	public:
		MainMenu(ApplicationState* appState);
		~MainMenu();

		void Show();

		inline UI::Menu& GetManager() { return menuManager; }
		inline UI::Menu* GetManagerPTR() { return &menuManager; }

	private:
		void Setup();

	private:
		UI::Menu menuManager;
		ApplicationState* appState = nullptr;
	};

}
