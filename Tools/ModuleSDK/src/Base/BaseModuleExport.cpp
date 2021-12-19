#include "Defines.h"

#include "BaseModuleExport.h"
#include "BaseModuleInterface.h"

#include <string>
#include <cstring>
#include <cstdlib>
#define MIN(x, y) x > y ? x : y

char* GetModuleName()
{
	char* name = (char*)malloc(1024 * 4);
	memcpy(name, GetName().data(), MIN(1024 * 4 - 1, GetName().size()));
	return name;
}

char* GetModuleVersion()
{
	char* version = (char*)malloc(1024 * 4);
	memcpy(version, GetVersion().data(), MIN(1024 * 4 - 1, GetVersion().size()));
	return version;
}

bool VerifyUpdate(char* path)
{
	return OnModuleUpdate(std::string(path));
}

void LoadModule()
{
	OnModuleLoad();
}

void UnloadModule()
{
	OnModuleUnload();
}