#pragma once
#include "UI/Editor.hpp"
#include "UI/Modals.hpp"

namespace TerraForge3D
{
	class ApplicationState;

	class StartUpScreen : public UI::Editor
	{
	public:
		StartUpScreen(ApplicationState* appState);
		~StartUpScreen();

		void OnUpdate() override;
		void OnShow() override;
		void OnStart() override;
		void OnEnd() override;

	private:
		ApplicationState* appState = nullptr;
		UI::FileDialogInfo openFDI, createFDI;
	};

}