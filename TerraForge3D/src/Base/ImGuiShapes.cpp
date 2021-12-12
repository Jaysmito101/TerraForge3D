#include "ImGuiShapes.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

#include <iostream>

void ImGui::DrawCircle(float radius, ImU32 color, float segments, float thickness)
{
	ImDrawList* list = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetCursorPos() + ImGui::GetWindowPos();
	list->AddCircle(ImVec2(pos.x + radius, pos.y + radius), radius, color, segments, thickness);
}

void ImGui::DrawFilledCircle(float radius, ImU32 color, float segments, float thickness)
{
	ImDrawList* list = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetCursorPos() + ImGui::GetWindowPos();
	list->AddCircleFilled(ImVec2(pos.x + radius, pos.y + radius), radius, color, segments);
}
