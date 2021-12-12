#pragma once

#include "imgui.h"

namespace ImGui 
{
	void DrawCircle(float radius = 5, ImU32 color = ImColor(255, 255, 255), float segments = 0, float thickness = 1.0);
	void DrawFilledCircle(float radius = 5, ImU32 color = ImColor(255, 255, 255), float segments = 0, float thickness = 1.0);
}
