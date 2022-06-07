#include "Data/ProjectManager.hpp"

#include "TerraForge3D.hpp"


namespace TerraForge3D
{



	ProjectManager::ProjectManager(ApplicationState* appState)
	{
		this->appState = appState;
	}

	ProjectManager::~ProjectManager()
	{
	}

	bool ProjectManager::Create(std::string path)
	{
		Utils::RmDir(path);
		Utils::MkDir(path);
		projectDir = path;
		Save();
		name = projectDir.substr(projectDir.find_last_of(PATH_SEPERATOR) + 1);
		appState->core.window->SetTitle(appState->core.app->applicationName + " [" + name + "]");
		isOpen = true;

		return true;
	}

	bool ProjectManager::Load(std::string path)
	{
		return false;
	}

	void ProjectManager::Close()
	{
		isOpen = false; appState->core.window->SetTitle(appState->core.app->applicationName);
	}

	void ProjectManager::Update()
	{
	}

	bool ProjectManager::Save()
	{
		TF3D_LOG_WARN("ProjectManager::Save yet to be implemented!");
		return false;
	}

}