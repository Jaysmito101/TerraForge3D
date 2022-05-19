#include "Data/Preferences.hpp"
#include "Data/ApplicationState.hpp"

namespace TerraForge3D
{




	Preferences::Preferences(ApplicationState* appState)
	{
		this->appState = appState;
	}

	Preferences::~Preferences()
	{
	}

	std::string Preferences::Save()
	{
		TF3D_LOG_WARN("Preferences::Save not yet implemented");
		return "";
	}

	bool Preferences::Load(std::string data)
	{
		TF3D_LOG_WARN("Preferences::Load not yet implemented");
		return true;
	}

	PreferencesEditor* Preferences::GetEditor()
	{
		if (!editor)
			editor = new PreferencesEditor(appState);
		return editor;
	}

}