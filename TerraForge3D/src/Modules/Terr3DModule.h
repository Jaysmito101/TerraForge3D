#pragma once
#include <string>
#include <unordered_map>
#include <iostream>

class ModuleManager;

namespace ModuleData {
	enum Type
	{
		NodeModule,
		UIModule,
		FilterModule,
		NoiseLayerModule
	};

}

class Module
{
public:
	std::string GetName();
	std::string GetVersion();
	bool VerifyUpdate(std::string path);

	Module(std::string id, ModuleManager* manager);
	~Module();

	ModuleData::Type type;
	std::unordered_map<std::string, void*> functions;
	std::string id;
};
