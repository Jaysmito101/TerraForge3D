#include "Data/ProjectData.h"
#include <Utils.h>
#include <json.hpp>

nlohmann::json projectDatase;

std::string projectID = "";

void SetProjectId(std::string id)
{
	projectID = id;
}

std::string GetProjectId()
{
	return projectID;
}

nlohmann::json GetProjectDatabase()
{
	return projectDatase;
}

void RegisterProjectAsset(std::string uid, std::string path)
{
	projectDatase[uid] = path;
}

std::string GetProjectAsset(std::string id)
{
	if(projectDatase.find(id) != projectDatase.end())
	{
		return projectDatase[id];
	}

	return "";
}

bool ProjectAssetExists(std::string id)
{
	return (projectDatase.find(id) != projectDatase.end());
}

void SetProjectDatabase(nlohmann::json db)
{
	projectDatase = db;
}

void SaveProjectDatabase()
{
	if (!PathExist(GetProjectResourcePath()))
	{
		MkDir(GetProjectResourcePath());
	}

	SaveToFile(GetProjectResourcePath() + "\\project_database.terr3d", projectDatase.dump());
}

std::string GetProjectResourcePath()
{
	return GetExecutableDir() + "\\Data\\cache\\project_data\\project_" + GetProjectId();
}

std::string SaveProjectTexture(Texture2D *texture)
{
	std::string path = texture->GetPath();

	if (path.size() <= 3)
	{
		return "";
	}

	std::string hash = MD5File(path).ToString();

	if (GetProjectAsset(hash).size() <= 0)
	{
		MkDir(GetProjectResourcePath() + "\\textures");
		CopyFileData(path, GetProjectResourcePath() + "\\textures\\" + hash);
		RegisterProjectAsset(hash, "textures\\" + hash);
	}

	return hash;
}