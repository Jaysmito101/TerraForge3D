#include "UIFontManager.h"

#include <unordered_map>

static std::unordered_map<std::string, ImFont*> fonts;

void LoadUIFont(std::string name, float pizelSize, std::string path)
{
	ImGuiIO& io = ImGui::GetIO();
	fonts[name] = io.Fonts->AddFontFromFileTTF(path.c_str(), pizelSize);
}

ImFont* GetUIFont(std::string name)
{
	if (fonts.find(name) != fonts.end())
		return fonts[name];
	return nullptr;
}
