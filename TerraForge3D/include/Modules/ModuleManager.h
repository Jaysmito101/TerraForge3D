#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "Module.h"

class ModuleManager
{
public:
	ModuleManager();

	~ModuleManager();

	void InstallModule(std::string path);

	void UninstallModule(std::string uid);

	void ShowSettings(bool *pOpen);

	void UpdateModules();

public:
	std::vector<Module *> loadedModules;
	std::unordered_map<std::string, Module *> modules;
};