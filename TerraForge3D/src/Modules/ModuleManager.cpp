#include "Modules/ModuleManager.h"
#include "Utils/Utils.h"

#include "zip.h"
#include "imgui.h"
#include "json.hpp"


ModuleManager::ModuleManager()
{
	// TODO
}

ModuleManager::~ModuleManager()
{
	for(Module *module : loadedModules)
	{
		module->OnUnload();
		delete module;
	}
}

void ModuleManager::InstallModule(std::string path)
{
	// TODO
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