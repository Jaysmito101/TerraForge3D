#pragma once
#include <imgui.h>

void LoadDefaultStyle();
void LoadDarkCoolStyle();
void LoadLightOrngeStyle();
void LoadBlackAndWhite();

bool LoadThemeFromFile(std::string filename);
void ShowStyleEditor(bool* pOpen);