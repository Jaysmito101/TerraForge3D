#include "Terr3DModule.h"
#include "ModuleManager.h"

std::string Module::GetName()
{
	return ((std::string(*)(void))(functions["GetName"]))();
}

std::string Module::GetVersion()
{
	return ((std::string(*)(void))(functions["GetVersion"]))();
}

bool Module::VerifyUpdate(std::string path)
{
	// To be used in future
	return true;
}

Module::Module(std::string id, ModuleManager *manager)
	:id(id)
{
	if (!manager->GetDLLHandle(id))
	{
		std::cout << "Failed to Load Module : " << id << std::endl;
	}

	functions["GetName"] = manager->GetFunctionPointer(id, "GetModuleName");
	functions["GetVersion"] = manager->GetFunctionPointer(id, "GetModuleVersion");
}

Module::~Module()
{
}
