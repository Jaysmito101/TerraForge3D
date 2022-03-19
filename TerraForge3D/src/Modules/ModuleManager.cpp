#include "Modules/ModuleManager.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"
#include "Platform.h"

#include "zip.h"
#include "imgui.h"
#include "json.hpp"

#include <filesystem>

#ifdef TERR3D_WIN32
#define MODULE_EXT ".dll"
#include "windows.h"
typedef Module* (__stdcall *GetModuleFunc)(char*, ApplicationState*);
#else
#define MODULE_EXT ".so"
#include <dlfcn.h>
typedef Module* (*GetModuleFunc)(char*, ApplicationState*);
#endif

ModuleManager::ModuleManager(ApplicationState *appState)
{
	this->appState = appState;
	LoadModules();
}

void ModuleManager::UnloadModules()
{
	for(Module *module : loadedModules)
	{
		module->OnUnload();
	#ifdef TERR3D_WIN32
		FreeLibrary((HMODULE)module->nativeHandle);
	#else
		dlclose(module->nativeHandle);
	#endif
		delete module;
	}
	loadedModules.clear();
}

void ModuleManager::LoadModules()
{
	UnloadModules();
	// iterate through all directories in the modules folder
	std::string modulesFolder = appState->constants.modulesDir + PATH_SEPERATOR;
	for (auto &p : std::filesystem::directory_iterator(modulesFolder))
	{
		std::string path = p.path().string();
		if (p.is_directory() && FileExists(path + PATH_SEPERATOR + "module" MODULE_EXT ))
		{
			LoadModule(path + PATH_SEPERATOR + "module" MODULE_EXT);
		}
	}
}

void ModuleManager::LoadModule(std::string path)
{
#ifdef TERR3D_WIN32
	HINSTANCE dll = LoadLibrary(s2ws(path).c_str());

	if(!dll)
	{
		std::cout << "Failed to load module: " << path << std::endl;
		return;
	}

	GetModuleFunc getModule = (GetModuleFunc)GetProcAddress(dll, "GetModule");

	if(!getModule)
	{
		std::cout << "Failed to get module function: " << path << std::endl;
		FreeLibrary(dll);
		return;
	}

	std::string uid = GenerateId(32);
	Module *module = getModule(uid.data(), appState);

	module->nativeHandle = dll;

#else
	void *handle;
	handle = dlopen(path.data(), RTLD_LAZY);
	if (!handle)
	{
		std::cout << "Failed to load module: " << path << std::endl;
		return;
	}
	dlerror();
	GetModuleFunc getModule = (GetModuleFunc)dlsym(handle, "GetModule");
	if (!getModule)
	{
		std::cout << "Failed to get module function: " << path << std::endl;
		dlclose(handle);
		return;
	}

	std::string uid = GenerateId(32);
	Module *module = getModule(uid.data(), appState);
	module->nativeHandle = handle;

#endif
}


ModuleManager::~ModuleManager()
{
	UnloadModules();
}

void ModuleManager::InstallModule(std::string path)
{
	// extract zip file to modules folder
	zip_extract(path.data(), (appState->constants.modulesDir + PATH_SEPERATOR + GenerateId(32)).data(), [](const char* f, void* arg){return 1;}, 0);
	// load module
	LoadModule(appState->constants.modulesDir + PATH_SEPERATOR + GenerateId(32) + PATH_SEPERATOR + "module" MODULE_EXT);
}

void ModuleManager::UninstallModule(std::string uid)
{
	if(modules.find(uid) == modules.end())
	{
		return;
	}

	Module *module = modules[uid];
	modules.erase(modules.find(uid));
	loadedModules.erase(std::find(loadedModules.begin(), loadedModules.end(), module));

	if(!module)
	{
		return;
	}

	module->OnUninstall();
}

void ModuleManager::ShowSettings(bool *pOpen)
{
	ImGui::Begin("Module Manager##TerraForge3DModuleManager", pOpen);
	ImGui::Text("Currently Installed Modules : %d", loadedModules.size());

	for(Module *module : loadedModules)
	{
		ImGui::PushID(module->uid.data());

		if(ImGui::CollapsingHeader(module->info.name.data()))
		{
			ImGui::Text("Name	 : %s", module->info.name.data());
			ImGui::Text("Author	 : %s", module->info.authorName.data());
			ImGui::Text("Version : %s", module->info.versionString.data());
			ImGui::Text("Website : %s", module->info.website.data());
			ImGui::Text("Contact : %s", module->info.contact.data());
			ImGui::Checkbox("Enabled", &module->isEnabled);
			ImGui::Text(module->info.description.data());
		}

		ImGui::PopID();
		ImGui::Separator();
	}

	ImGui::End();
	void *imguiContext = static_cast<void *>(ImGui::GetCurrentContext());

	for(Module *module : loadedModules)
	{
		module->RenderImGui(imguiContext);
	}
}

void ModuleManager::UpdateModules()
{
	for(Module *module : loadedModules)
	{
		module->Update();
	}
}