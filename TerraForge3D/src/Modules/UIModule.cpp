#include "UIModule.h"
#include "ModuleManager.h"
#include "imgui.h"

UIModule::UIModule(std::string id, ModuleManager* manager)
	:Module(id, manager)
{
	type = ModuleData::Type::UIModule;
	functions["Render"] = manager->GetFunctionPointer(id, "Render");
	functions["GetWindowName"] = manager->GetFunctionPointer(id, "GetWindowName");
	windowName = ((char*(*)(void))(functions["GetWindowName"]))();
}

void UIModule::Render() 
{
	((void(*)(void*))(functions["Render"]))(ImGui::GetCurrentContext());
}
