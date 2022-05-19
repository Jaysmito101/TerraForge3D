#include "Data/PreferencesEditor.hpp"
#include "Data/Preferences.hpp"
#include "Data/ApplicationState.hpp"
#include "UI/MainMenu.hpp"

#include "imgui/imgui.h"

namespace TerraForge3D
{
	PreferencesEditor::PreferencesEditor(ApplicationState* appState)
		: UI::Editor("Preferences")
	{
		this->appState = appState;
		this->preferences = appState->preferences;
	}

	PreferencesEditor::~PreferencesEditor()
	{
	}

	void PreferencesEditor::OnStart()
	{
		isVisible = false;
		appState->menus.mainMenu->GetManagerPTR()->Register("Edit/Preferences", [this](UI::MenuItem* item)->void { isVisible = true;});
		
		// Setup some dummy pannels
		PreferencesEditorTab tab;
		tab.name = "General";
		RegisterTab(tab);
		tab.name = "Addons";
		RegisterTab(tab);
		tab.name = "Account";
		RegisterTab(tab);
		tab.name = "Theme";
		RegisterTab(tab);
	}

	void PreferencesEditor::OnUpdate()
	{
		for (auto& [id, tab] : tabs)
		{
			if (tab.onUpdate)
			{
				tab.requireSave = tab.onUpdate(preferences, &tab);
			}
		}
	}

	void PreferencesEditor::OnShow()
	{
		// Left Panel
		char buffer[256];
		ImGui::BeginChild("TabsLeftPanel", ImVec2(150, 0));
		for (auto& [id, tab] : tabs)
		{
			sprintf(buffer, "%s##%d", tab.name.data(), id);
			if (ImGui::Selectable(buffer, selectedTab == id))
			{
				selectedTab = id;
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Right Panel
		ImGui::BeginChild("TabsRightPanel");
		PreferencesEditorTab& tab = tabs[selectedTab];
		ImGui::PushID(selectedTab);
		if (tab.onShow)
			tab.requireSave = tab.onShow(preferences, &tab);
		ImGui::PopID();
		if (!tab.requireSave)
			ImGui::BeginDisabled();
		// if (ImGui::Button("Save"))
		if (Utils::ImGuiC::ButtonIcon(ICON_MD_SAVE, "Save"))
			Save();
		ImGui::SameLine();
		if (Utils::ImGuiC::ButtonIcon(ICON_MD_FILE_OPEN, "Load from disk"))
			Load();
		ImGui::SameLine();
		if (Utils::ImGuiC::ButtonIcon(ICON_MD_REFRESH, "Reset"))
			Reset(selectedTab);
		if (!tab.requireSave)
			ImGui::EndDisabled();
		ImGui::EndChild();
	}

	void PreferencesEditor::OnEnd()
	{
		appState->menus.mainMenu->GetManagerPTR()->Deregister("Edit/Preferences");
	}

	bool PreferencesEditor::Save()
	{
		std::string data = preferences->Save();
		return Utils::WriteTextFile(appState->appResourcePaths.preferencesPath, data);
	}

	bool PreferencesEditor::Load()
	{
		bool success = false;
		std::string data = Utils::ReadTextFile(appState->appResourcePaths.preferencesPath, &success);
		if(success)
			return preferences->Load(data);
		return success;
	}

	bool PreferencesEditor::Reset(uint32_t tabID)
	{
		if (tabs[tabID].reset)
		{
			return tabs[tabID].reset(preferences);
		}
		else
			TF3D_LOG_WARN("Reset function not found for preferences tab {0}", tabs[tabID].name);
		return false;
	}

	uint32_t PreferencesEditor::RegisterTab(PreferencesEditorTab tab)
	{
		// TF3D_ASSERT(tabs.find(tab.name) == tabs.end(), "Tab with same name already registered");
		static uint32_t tabID = 0;
		if(tab.onCreate)
			tab.onCreate(&tab);
		tabs[++tabID] = tab;
		return tabID;
	}

	bool PreferencesEditor::DeregisterTab(uint32_t tab)
	{
		if (tabs.find(tab) == tabs.end())
		{
			TF3D_LOG_WARN("Tab id {0} not registered", tab);
			return false;
		}
		if (tabs[tab].onDelete)
			tabs[tab].onDelete(&tabs[tab]);
		tabs.erase(tab);
		return true;
	}
}