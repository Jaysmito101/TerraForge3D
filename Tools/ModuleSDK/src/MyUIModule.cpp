// This is a smaple module

// Author : Jaysmito Mukherjee
// Name : MyUIModuleExample

#include "imgui.h"

#include "Terr3DModule.h"

void OnModuleLoad()
{
}

void OnModuleUnload()
{
}

std::string GetName()
{
	return "MyUIModuleExample";
}

std::string GetVersion()
{
	return "1.0";
}

bool OnModuleUpdate(std::string path)
{
	// This function is for future use.
	return true;
}

void OnRender()
{
	ImGui::Text("Hello World!");
}

