#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "UIModule.h"
#include "NoiseLayerModule.h"
#include "NodeModule.h"
#include "Terr3DModule.h"

class ModuleManager
{
public:
	ModuleManager();
	~ModuleManager();

	void Render();
	void InstallModule(std::string path);
	void UinstallModule(std::string id);
	void *GetDLLHandle(std::string id);
	void *GetFunctionPointer(std::string id, std::string functionName);
	NodeModule *FindNodeModule(std::string id);

	std::unordered_map<std::string, void *> dllHandles;
	std::vector<UIModule *> uiModules;
	std::vector<NoiseLayerModule *> nlModules;
	std::vector<NodeModule *> noModules;
	nlohmann::json modData;
};