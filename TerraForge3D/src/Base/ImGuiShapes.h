#pragma once

#include "imgui.h"

namespace ImGui 
{
	void DrawCircle(float radius = 5, ImU32 color = ImColor(255, 255, 255), float segments = 0, float thickness = 1.0);
	void DrawFilledCircle(float radius = 5, ImU32 color = ImColor(255, 255, 255), float segments = 0);
	void DrawRect(ImVec2 size = ImVec2(10, 10), ImU32 color = ImColor(255, 255, 255), float rounding = 0.0f, float thickness = 1.0f);
	void DrawFilledRect(ImVec2 size = ImVec2(10, 10), ImU32 color = ImColor(255, 255, 255), float rounding = 0.0f);
}
