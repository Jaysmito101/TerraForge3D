#include "Data/ProjectData.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"
#include "Platform.h"

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
		std::string path = projectDatase[id];
#ifndef TERR3D_WIN32
		// replace \ with /
		for(int i = 0; i < path.size(); i++)
		{
			if(path[i] == '\\')
			{
				path[i] = '/';
			}
		}
#endif
		return path;
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

	SaveToFile(GetResourcePath() + PATH_SEPARATOR "project_database.terr3d", projectDatase.dump());
}

std::string ProjectManager::GetResourcePath()
{
	return GetExecutableDir() + PATH_SEPARATOR "Data" PATH_SEPARATOR "cache" PATH_SEPARATOR "project_data" PATH_SEPARATOR "project_" + GetId();
}

std::string ProjectManager::SaveTexture(Texture2D *texture)
{
	return SaveResource("textures", texture->GetPath());
}

std::string ProjectManager::SaveResource(std::string folder, std::string path)
{
	if (path.size() <= 3)
	{
		return "";
	}

	std::string hash = MD5File(path).ToString();

	if (GetAsset(hash).size() <= 0)
	{
		MkDir(GetResourcePath() + PATH_SEPARATOR + folder);
		CopyFileData(path, GetResourcePath() + PATH_SEPARATOR + folder + PATH_SEPARATOR + hash);
		RegisterAsset(hash, folder + PATH_SEPARATOR + hash);
	}

	return hash;
}
