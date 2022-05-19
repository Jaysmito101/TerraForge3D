#pragma once
#include "Base/Core/Core.hpp"
#include "Data/PreferencesEditor.hpp"

namespace TerraForge3D
{
	class ApplicationState;

	class Preferences
	{
	public:
		Preferences(ApplicationState* appState);
		~Preferences();

		std::string Save();
		bool Load(std::string data);

		PreferencesEditor* GetEditor();

	public:

	private:
		PreferencesEditor* editor = nullptr;
		ApplicationState* appState = nullptr;
	};

}