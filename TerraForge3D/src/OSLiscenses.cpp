#include "OSLiscenses.h"

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING


#include "imgui.h"
#include <string>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <experimental/filesystem>
#include <Utils.h>
namespace fs = std::experimental::filesystem;

std::vector<std::pair<std::string, std::string>> osls;

void SetupOSLiscences()
{
	std::string path = GetExecutableDir() + "\\Data\\licenses";

	for (const auto &entry : fs::directory_iterator(path))
	{
		std::string path{ entry.path().u8string() };
		std::string name{entry.path().filename().u8string()};
		bool tmp = false;
		name = name.substr(0, name.size() - 3);
		osls.push_back(std::make_pair(name, ReadShaderSourceFile(path, &tmp)));
	}
}

static void ShowLisc(std::string &name, std::string &content, int id)
{
	bool state = ImGui::CollapsingHeader(("##LiscItem" + std::to_string(id)).c_str());
	ImGui::SameLine();
	ImGui::Text(name.c_str());

	if (state)
	{
		ImGui::Text(content.c_str());
	}
}


void ShowOSLiscences(bool *pOpen)
{
	ImGui::Begin("Open Source LICENSES", pOpen);
	int id = 0;

	for (auto &item : osls)
	{
		ShowLisc(item.first, item.second, id++);
	}

	ImGui::End();
}
