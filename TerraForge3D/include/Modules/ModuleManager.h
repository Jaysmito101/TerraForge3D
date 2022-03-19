#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "Module.h"

struct ApplicationState;

class ModuleManager
{
public:
	ModuleManager(ApplicationState *appState);

	~ModuleManager();

	void InstallModule(std::string path);

	void UninstallModule(std::string uid);

	void ShowSettings(bool *pOpen);

	void UpdateModules();

private:
	void LoadModules();
	void UnloadModules();
	void LoadModule(std::string path);

public:
	ApplicationState *appState;
	std::vector<Module *> loadedModules;
	std::unordered_map<std::string, Module *> modules;
};