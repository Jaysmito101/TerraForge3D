#pragma once
#include <imgui.h>
#include <string>

void LoadDefaultStyle();
void LoadDarkCoolStyle();
void LoadLightOrngeStyle();
void LoadBlackAndWhite();
void LoadMayaStyle();
void ShowStyleEditor(bool *pOpen);

std::string GetCurrentThemeName();
void SetCurrentThemeName(const std::string& name);
void CaptureCurrentThemeDefaults();
void ResetCurrentThemeToDefaults();

bool LoadThemeFromFile(std::string filename);
bool LoadThemeFromStr(std::string data);
std::string GetStyleData();
