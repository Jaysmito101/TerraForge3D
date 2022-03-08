#pragma once

#include <string>
#include "Base/Base.h"

void SetProjectId(std::string id);

std::string GetProjectId();

nlohmann::json GetProjectDatabase();

void RegisterProjectAsset(std::string uid, std::string path);

std::string GetProjectAsset(std::string id);

bool ProjectAssetExists(std::string id);

void SetProjectDatabase(nlohmann::json db);

void SaveProjectDatabase();

std::string GetProjectResourcePath();

std::string SaveProjectTexture(Texture2D *texture);