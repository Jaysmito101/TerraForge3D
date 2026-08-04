#include "UI/ImGuiComponents.h"

#include "Base/Base.h"

#include <algorithm>

bool PowerOfTwoDropDown(const char* label, int32_t* value, int start, int end)
{
	if (!value) return false;
	static char buffer[32];
	int tmp = static_cast<int>(log(static_cast<double>(*value)) / log(2.0));
	snprintf(buffer, 32, "%d", static_cast<int>(pow(2, tmp)));
	if (ImGui::BeginCombo(label, buffer))
	{
		for (int i = start; i <= end; i++)
		{
			bool isSelected = (tmp == i);
			snprintf(buffer, 32, "%d", static_cast<int>(pow(2, i)));
			if (ImGui::Selectable(buffer, isSelected)) tmp = i;
			if (isSelected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	tmp = static_cast<int>(pow(2, tmp));
	const bool changed = (*value != tmp);
	*value = tmp;
	return changed;
}

bool ShowComboBox(const char* label, int* selected, const char** values, int count)
{
	if (!selected || !values || count <= 0) return false;
	*selected = std::clamp(*selected, 0, count - 1);
	int current = *selected;
	if (ImGui::BeginCombo(label, values[current]))
	{
		for (int i = 0; i < count; i++)
		{
			const bool isSelected = (current == i);
			if (ImGui::Selectable(values[i], isSelected)) current = i;
			if (isSelected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	const bool changed = (*selected != current);
	*selected = current;
	return changed;
}

bool ShowLayerUpdationMethod(const char* label, int* method)
{
	if (!method) return false;
	static const char* items[] = { "Set", "Add", "Subtract", "Multiply" };
	*method = std::clamp(*method, 0, IM_ARRAYSIZE(items) - 1);
	int selected = *method;
	if (ImGui::BeginCombo(label, items[selected]))
	{
		for (int i = 0; i < IM_ARRAYSIZE(items); i++)
		{
			const bool isSelected = (selected == i);
			if (ImGui::Selectable(items[i], isSelected)) selected = i;
			if (isSelected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	const bool changed = (*method != selected);
	*method = selected;
	return changed;
}

bool ShowSeedSettings(const std::string& label, int* seed, std::vector<int>& historyStack)
{
	if (!seed) return false;

	bool changed = false;
	const auto savedSeed = std::find(historyStack.begin(), historyStack.end(), *seed);
	const bool isSaved = savedSeed != historyStack.end();

	ImGui::PushID(label.c_str());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, ImGui::GetStyle().ItemSpacing.y));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label.c_str());
	ImGui::SameLine();

	const float buttonPadding = ImGui::GetStyle().FramePadding.x * 2.0f;
	const float randomWidth = ImGui::CalcTextSize("Random").x + buttonPadding;
	const float savedWidth = ImGui::CalcTextSize(isSaved ? "Saved" : "Save").x + buttonPadding;
	const std::string historyLabel = "History (" + std::to_string(historyStack.size()) + ")";
	const float historyWidth = ImGui::CalcTextSize(historyLabel.c_str()).x + buttonPadding;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float availableWidth = ImGui::GetContentRegionAvail().x;
	const float inputWidth = std::max(76.0f, availableWidth - randomWidth - savedWidth - historyWidth - spacing * 3.0f);

	ImGui::SetNextItemWidth(inputWidth);
	if (ImGui::InputInt("##SeedValue", seed, 1, 10)) changed = true;
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enter a seed value or use the +/- controls");

	ImGui::SameLine();
	if (ImGui::Button("Random"))
	{
		*seed = rand() % 500;
		changed = true;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Generate a new random seed");

	ImGui::SameLine();
	if (isSaved) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	if (ImGui::Button(isSaved ? "Saved" : "Save"))
	{
		if (isSaved) historyStack.erase(savedSeed);
		else historyStack.push_back(*seed);
	}
	if (isSaved) ImGui::PopStyleColor();
	if (ImGui::IsItemHovered()) ImGui::SetTooltip(isSaved ? "Remove this seed from saved history" : "Save this seed for quick reuse");

	ImGui::SameLine();
	if (ImGui::Button(historyLabel.c_str())) ImGui::OpenPopup("##SeedHistory");
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Choose from saved seeds");

	if (ImGui::BeginPopup("##SeedHistory"))
	{
		ImGui::TextUnformatted("Saved seeds");
		ImGui::Separator();
		if (historyStack.empty())
		{
			ImGui::TextDisabled("No saved seeds yet");
		}
		else
		{
			for (auto iterator = historyStack.rbegin(); iterator != historyStack.rend(); ++iterator)
			{
				const std::string seedLabel = std::to_string(*iterator);
				if (ImGui::Selectable(seedLabel.c_str(), *seed == *iterator))
				{
					*seed = *iterator;
					changed = true;
					ImGui::CloseCurrentPopup();
				}
			}
		}
		ImGui::Separator();
		if (ImGui::Button("Clear saved seeds") && !historyStack.empty()) historyStack.clear();
		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
	ImGui::PopID();
	return changed;
}
