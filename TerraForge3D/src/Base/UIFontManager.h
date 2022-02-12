#pragma once

#include "imgui.h"
#include <string>

void LoadUIFont(std::string name, float pixelSize, std::string path);
ImFont* GetUIFont(std::string name);
