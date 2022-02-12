#include "Defines.h"
#ifdef UI_MODULE

#include "UIModuleExport.h"
#include "UIModuleInterface.h"
#include "BaseModuleInterface.h"
#include "json.hpp"
#include "imgui.h"

void Render(void* imguiContext)
{
	ImGui::SetCurrentContext((ImGuiContext*)imguiContext);
	OnRender();
}

char* GetWindowName()
{
	std::string dataS = GetName();
	char* data = (char*)malloc(dataS.size());
	memcpy(data, dataS.data(), dataS.size());
	return data;
}

#endif