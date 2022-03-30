#pragma once

#include <string>

#include "Base/Texture2D.h"
#include "Base/Heightmap.h"
#include "json/json.hpp"

class ApplicationState;

class ProjectManager
{
public:
	ProjectManager(ApplicationState *appState);
	~ProjectManager();

	void SetId(std::string id);

	std::string GetId();

	nlohmann::json GetDatabase();

	void RegisterAsset(std::string uid, std::string path);

	std::string GetAsset(std::string id);

	bool AssetExists(std::string id);

	void SetDatabase(nlohmann::json db);

	void SaveDatabase();

	std::string GetResourcePath();

	std::string SaveTexture(Texture2D *texture);
	std::string SaveResource(std::string folder, std::string path);

	static ProjectManager *Get();

public:
	ApplicationState *appState = nullptr;
	std::string projectID = "";
	nlohmann::json projectDatase;
private:
	static ProjectManager *s_ProjectManager;
};