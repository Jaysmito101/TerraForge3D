#include "Data/Preferences.hpp"
#include "TerraForge3D.hpp"


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

	SharedPtr<PreferencesEditor> Preferences::GetEditor()
	{
		if (!editor)
			editor = new PreferencesEditor(appState);
		return editor;
	}

}