#pragma once
#include <imgui.h>

void LoadDefaultStyle();
void LoadDarkCoolStyle();
void LoadLightOrngeStyle();
void LoadBlackAndWhite();
void ShowStyleEditor(bool* pOpen);

bool LoadThemeFromFile(std::string filename);
bool LoadThemeFromStr(std::string data);
std::string GetStyleData();