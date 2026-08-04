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
	const ImGuiStyle& style = ImGui::GetStyle();
	const float height = ImGui::GetFrameHeight();
	const float gap = style.ItemInnerSpacing.x;
	const float controlWidth = ImGui::CalcItemWidth();
	const float minimumValueWidth = 64.0f;
	float actionSize = height;
	if (controlWidth < minimumValueWidth + actionSize * 3.0f + gap * 3.0f)
		actionSize = std::max(18.0f, (controlWidth - minimumValueWidth - gap * 3.0f) / 3.0f);

	const float valueWidth = std::max(1.0f, controlWidth - actionSize * 3.0f - gap * 3.0f);
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	auto drawPanel = [&](const ImVec2& min, const ImVec2& max, bool hovered, bool active, bool selected, bool action)
	{
		ImGuiCol backgroundColor = action ? ImGuiCol_Button : ImGuiCol_FrameBg;
		if (selected || active) backgroundColor = action ? ImGuiCol_ButtonActive : ImGuiCol_FrameBgActive;
		else if (hovered) backgroundColor = action ? ImGuiCol_ButtonHovered : ImGuiCol_FrameBgHovered;
		drawList->AddRectFilled(min, max, ImGui::GetColorU32(backgroundColor), style.FrameRounding);
	};

	ImGui::InvisibleButton("##SeedValue", ImVec2(valueWidth, height));
	const ImVec2 valueMin = ImGui::GetItemRectMin();
	const ImVec2 valueMax = ImGui::GetItemRectMax();
	const bool valueHovered = ImGui::IsItemHovered();
	const bool valueActive = ImGui::IsItemActive();
	drawPanel(valueMin, valueMax, valueHovered, valueActive, false, false);

	const std::string seedText = std::to_string(*seed);
	const ImVec2 seedTextSize = ImGui::CalcTextSize(seedText.c_str());
	drawList->AddText(
		ImVec2(valueMin.x + (valueWidth - seedTextSize.x) * 0.5f, valueMin.y + (height - seedTextSize.y) * 0.5f),
		ImGui::GetColorU32(ImGuiCol_Text), seedText.c_str());

	if (valueActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		const int delta = static_cast<int>(ImGui::GetIO().MouseDelta.x);
		if (delta != 0)
		{
			*seed += delta;
			changed = true;
		}
	}
	if (valueHovered) ImGui::SetTooltip("Drag to adjust • Double-click to enter an exact value");
	if (valueHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) ImGui::OpenPopup("##SeedEdit");

	ImGui::SameLine(0.0f, gap);
	ImGui::InvisibleButton("##SeedRandom", ImVec2(actionSize, height));
	const ImVec2 randomMin = ImGui::GetItemRectMin();
	const ImVec2 randomMax = ImGui::GetItemRectMax();
	const bool randomHovered = ImGui::IsItemHovered();
	drawPanel(randomMin, randomMax, randomHovered, ImGui::IsItemActive(), false, true);
	const ImVec2 randomCenter((randomMin.x + randomMax.x) * 0.5f, (randomMin.y + randomMax.y) * 0.5f);
	const ImU32 iconColor = ImGui::GetColorU32(ImGuiCol_Text);
	drawList->PathClear();
	drawList->PathArcTo(randomCenter, 7.0f, -0.75f, 4.5f, 18);
	drawList->PathStroke(iconColor, false, 1.6f);
	drawList->AddTriangleFilled(
		ImVec2(randomCenter.x + 5.0f, randomCenter.y - 6.0f),
		ImVec2(randomCenter.x + 8.0f, randomCenter.y - 1.0f),
		ImVec2(randomCenter.x + 2.0f, randomCenter.y - 1.5f), iconColor);
	if (randomHovered) ImGui::SetTooltip("Generate a random seed");
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		*seed = rand() % 500;
		changed = true;
	}

	ImGui::SameLine(0.0f, gap);
	ImGui::InvisibleButton("##SeedSave", ImVec2(actionSize, height));
	const ImVec2 saveMin = ImGui::GetItemRectMin();
	const ImVec2 saveMax = ImGui::GetItemRectMax();
	const bool saveHovered = ImGui::IsItemHovered();
	drawPanel(saveMin, saveMax, saveHovered, ImGui::IsItemActive(), isSaved, true);
	const float bookmarkLeft = saveMin.x + actionSize * 0.34f;
	const float bookmarkRight = saveMin.x + actionSize * 0.66f;
	const float bookmarkTop = saveMin.y + actionSize * 0.27f;
	const float bookmarkBottom = saveMin.y + actionSize * 0.72f;
	drawList->PathClear();
	drawList->PathLineTo(ImVec2(bookmarkLeft, bookmarkTop));
	drawList->PathLineTo(ImVec2(bookmarkRight, bookmarkTop));
	drawList->PathLineTo(ImVec2(bookmarkRight, bookmarkBottom));
	drawList->PathLineTo(ImVec2((bookmarkLeft + bookmarkRight) * 0.5f, bookmarkBottom - 4.0f));
	drawList->PathLineTo(ImVec2(bookmarkLeft, bookmarkBottom));
	if (isSaved) drawList->PathFillConvex(ImGui::GetColorU32(ImGuiCol_Text));
	else drawList->PathStroke(iconColor, true, 1.5f);
	if (saveHovered) ImGui::SetTooltip(isSaved ? "Remove this seed from saved seeds" : "Save this seed for quick reuse");
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		if (isSaved) historyStack.erase(savedSeed);
		else historyStack.push_back(*seed);
	}

	ImGui::SameLine(0.0f, gap);
	ImGui::InvisibleButton("##SeedHistory", ImVec2(actionSize, height));
	const ImVec2 historyMin = ImGui::GetItemRectMin();
	const ImVec2 historyMax = ImGui::GetItemRectMax();
	const bool historyHovered = ImGui::IsItemHovered();
	drawPanel(historyMin, historyMax, historyHovered, ImGui::IsItemActive(), false, true);
	const ImVec2 historyCenter((historyMin.x + historyMax.x) * 0.5f, (historyMin.y + historyMax.y) * 0.5f);
	drawList->AddCircle(historyCenter, 7.0f, iconColor, 16, 1.5f);
	drawList->AddLine(historyCenter, ImVec2(historyCenter.x, historyCenter.y - 4.0f), iconColor, 1.5f);
	drawList->AddLine(historyCenter, ImVec2(historyCenter.x + 4.0f, historyCenter.y + 2.0f), iconColor, 1.5f);
	if (!historyStack.empty())
	{
		const std::string countText = std::to_string(historyStack.size());
		const ImVec2 countSize = ImGui::CalcTextSize(countText.c_str());
		drawList->AddText(ImVec2(historyMax.x - countSize.x - 3.0f, historyMin.y + 2.0f), iconColor, countText.c_str());
	}
	if (historyHovered) ImGui::SetTooltip("Open saved seed history");
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) ImGui::OpenPopup("##SeedHistoryPopup");

	ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label.c_str());

	if (ImGui::BeginPopup("##SeedEdit"))
	{
		ImGui::TextUnformatted("Seed value");
		ImGui::SetNextItemWidth(140.0f);
		if (ImGui::InputInt("##SeedEditValue", seed, 1, 10)) changed = true;
		if (ImGui::IsItemDeactivatedAfterEdit()) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("##SeedHistoryPopup"))
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

	ImGui::PopID();
	return changed;
}
