#include "Modules/Module.h"
#include "Modules/ModuleManager.h"
#include "Data/ApplicationState.h"

#include "imgui/imgui.h"

Module::Module(std::string id, ApplicationState *as)
{
	uid = id;
	appState = as;
}

Module::~Module()
{
}

// As for now this does nothing but it is for future use
void Module::Update()
{
	OnUpdate();
}

void Module::RenderImGui(void *imguiContext)
{
	ImGui::SetCurrentContext(static_cast<ImGuiContext *>(imguiContext));
	OnImGuiRender();
}

void Module::OnInstall()
{
}

void Module::OnUninstall()
{
}