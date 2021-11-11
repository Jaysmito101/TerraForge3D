#pragma once

#include <string>

void SetProjectId(std::string id);

std::string GetProjectId();

std::string GetProjectDatabase();

void RegisterProjectAsset(std::string uid, std::string path);

std::string GetProjectAsset(std::string id);

void SetProjectDatabase(std::string db);

void SaveProjectDatabase();

std::string GetProjectResourcePath();