#pragma once

#include "imgui.h"

namespace ImGui
{
    int Curve(const char* label, const ImVec2& size, int maxpoints, ImVec2* points);
    float CurveValue(float p, int maxpoints, const ImVec2* points);
    float CurveValueSmooth(float p, int maxpoints, const ImVec2* points);
}; // namespace ImGui