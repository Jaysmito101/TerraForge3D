#include "Data/ProjectData.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

#include <json/json.hpp>


ProjectManager *ProjectManager::s_ProjectManager;

ProjectManager *ProjectManager::Get()
{
	return s_ProjectManager;
}

ProjectManager::~ProjectManager()
{
}

ProjectManager::ProjectManager(ApplicationState *as)
{
	appState = as;
	s_ProjectManager = this;
}

void ProjectManager::SetId(std::string id)
{
	projectID = id;
}

std::string ProjectManager::GetId()
{
	return projectID;
}

nlohmann::json ProjectManager::GetDatabase()
{
	return projectDatase;
}

void ProjectManager::RegisterAsset(std::string uid, std::string path)
{
	projectDatase[uid] = path;
}

std::string ProjectManager::GetAsset(std::string id)
{
	if(projectDatase.find(id) != projectDatase.end())
	{
		return projectDatase[id];
	}

	return "";
}

bool ProjectManager::AssetExists(std::string id)
{
	return (projectDatase.find(id) != projectDatase.end());
}

void ProjectManager::SetDatabase(nlohmann::json db)
{
	projectDatase = db;
}

void ProjectManager::SaveDatabase()
{
	if (!PathExist(GetResourcePath()))
	{
		MkDir(GetResourcePath());
	}

	SaveToFile(GetResourcePath() + "\\project_database.terr3d", projectDatase.dump());
}

std::string ProjectManager::GetResourcePath()
{
	return GetExecutableDir() + "\\Data\\cache\\project_data\\project_" + GetId();
}

std::string ProjectManager::SaveTexture(Texture2D *texture)
{
	std::string path = texture->GetPath();

	if (path.size() <= 3)
	{
		return "";
	}

	std::string hash = MD5File(path).ToString();

	if (GetAsset(hash).size() <= 0)
	{
		MkDir(GetResourcePath() + "\\textures");
		CopyFileData(path, GetResourcePath() + "\\textures\\" + hash);
		RegisterAsset(hash, "textures\\" + hash);
	}

	return hash;
}