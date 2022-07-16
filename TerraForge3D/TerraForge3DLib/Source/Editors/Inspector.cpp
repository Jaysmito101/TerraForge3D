#include "Editors/Inspector.hpp"
#include "Data/ApplicationState.hpp"

#include "imgui/imgui_internal.h"

namespace TerraForge3D
{

	

	Inspector::Inspector(ApplicationState* as)
	{
		this->appState = as;
	}

	Inspector::~Inspector()
	{
	}

	void Inspector::OnStart()
	{
	}

	void Inspector::OnShow()
	{
		if (ImGui::GetWindowHeight() < itemViewHeight)
			itemViewHeight = ImGui::GetWindowHeight() - separatorSliderWidth;

		float windowWidth = ImGui::GetWindowWidth();
		float windowHeight = ImGui::GetWindowHeight();

		ImGui::BeginChild("##InspectorItemDetails", ImVec2(windowWidth, itemViewHeight));
		Utils::ImGuiC::PushSubFont(appState->core.fonts["SemiHeader"].handle);
		ImGui::Text("Selected Item Details");
		Utils::ImGuiC::PopSubFont();

		if (isTerrainSelected)
		{
			ImGui::Text("Selected terrain");
		}
		else
		{
			ImGui::Text("Select an item from the list below so see relevant options");
		}

		ImGui::EndChild();

		ImGuiMouseCursor currentCursor = ImGui::GetMouseCursor();
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
		ImGui::Button("##InspectorSeperator", ImVec2(windowWidth, separatorSliderWidth));
		float currentMousePos = ImGui::GetMousePos().y;
		float mouseDelta = currentMousePos - prevMousePos;
		prevMousePos = currentMousePos;
		//if (fabs(mouseDelta) > 50.0f)
			//mouseDelta = 50.0f;
		if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			itemViewHeight += mouseDelta;
			if (itemViewHeight <= separatorSliderWidth)
				itemViewHeight = separatorSliderWidth;
			if (itemViewHeight > ImGui::GetWindowHeight() - separatorSliderWidth)
				itemViewHeight = static_cast<uint32_t>(windowHeight - separatorSliderWidth);
		}
		ImGui::SetMouseCursor(currentCursor);

		ImGui::BeginChild("##InspectorList", ImVec2(windowHeight, ImGui::GetWindowHeight() - itemViewHeight - separatorSliderWidth - 30));
		Utils::ImGuiC::PushSubFont(appState->core.fonts["SemiHeader"].handle);
		ImGui::Text("Objects");
		Utils::ImGuiC::PopSubFont();

		ImGui::Selectable("Terrain", &isTerrainSelected, 0, ImVec2(windowWidth, 20.0f));

		ImGui::EndChild();
	}

	void Inspector::OnUpdate()
	{
	}

	void Inspector::OnEnd()
	{
	}

	bool Inspector::OnContextMenu()
	{
		return false;
	}

}