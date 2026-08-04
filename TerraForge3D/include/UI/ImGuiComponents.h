#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct ImGuiOctaveStrengthProfile
{
	float* values = nullptr;
	std::size_t count = 0;
	float minimum = 0.0f;
	float maximum = 1.0f;
};

bool ShowSeedSettings(const std::string& label, int* seed, std::vector<int>& historyStack);
bool ShowComboBox(const char* label, int* selected, const char** values, int count);
bool PowerOfTwoDropDown(const char* label, int32_t* value, int start, int end);
bool ShowLayerUpdationMethod(const char* label, int* method);
bool DrawOctaveStrengthProfile(const char* label, const ImGuiOctaveStrengthProfile& profile);

#define SHOW_COMBO_BOX(label, selected, values, count) \
{ \
	int p_VPModeCopy##__LINE__ = static_cast<int>(selected); \
	ShowComboBox(label, &p_VPModeCopy##__LINE__, values, count); \
	selected = static_cast<decltype(selected)>(p_VPModeCopy##__LINE__); \
}
