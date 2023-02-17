#include "Misc/OSLiscences.h"

#include "imgui.h"

#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

#include "Utils/Utils.h"
#include "Data/ApplicationState.h"

namespace fs = std::filesystem;

OSLiscences::~OSLiscences()
{
	osls.clear();
}

OSLiscences::OSLiscences(ApplicationState *as)
{
	appState = as;
	std::string path = appState->constants.liscensesDir;

	for (const auto &entry : fs::directory_iterator(path))
	{
		std::string path{ entry.path().u8string() };
		std::string name{ entry.path().filename().u8string()};
		bool tmp = false;
		name = name.substr(0, name.size() - 3);
		osls.push_back(std::make_pair(name, ReadShaderSourceFile(path, &tmp)));
	}
}

void OSLiscences::ShowLisc(std::string &name, std::string &content, int id)
{
	bool state = ImGui::CollapsingHeader(("##LiscItem" + std::to_string(id)).c_str());
	ImGui::SameLine();
	ImGui::Text(name.c_str());

	if (state)
	{
		ImGui::Text(content.c_str());
	}
}


void OSLiscences::ShowSettings(bool *pOpen)
{
	ImGui::Begin("Open Source LICENSES", pOpen);
	int id = 0;

	for (auto &item : osls)
	{
		ShowLisc(item.first, item.second, id++);
	}

	ImGui::End();
}
