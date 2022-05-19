#pragma once
#include "Base/Core/Core.hpp"
#include "UI/Editor.hpp"

namespace TerraForge3D
{
	class Preferences;
	class ApplicationState;

	struct PreferencesEditorTab
	{
		std::string name = "PreferencesTab";
		void* userData = nullptr;
		std::function<bool(Preferences*, PreferencesEditorTab*)> onShow = nullptr;
		std::function<bool(Preferences*, PreferencesEditorTab*)> onUpdate = nullptr;
		std::function<bool(Preferences*)> reset = nullptr;
		std::function<void(PreferencesEditorTab*)> onCreate = nullptr;
		std::function<void(PreferencesEditorTab*)> onDelete = nullptr;
		bool requireSave = false;
	};

	class PreferencesEditor : public UI::Editor
	{
	public:
		PreferencesEditor(ApplicationState* appState);
		~PreferencesEditor();

		virtual void OnStart() override;
		virtual void OnUpdate() override;
		virtual void OnShow() override;
		virtual void OnEnd() override;

		bool Save();
		bool Load();
		bool Reset(uint32_t tabId);

		uint32_t RegisterTab(PreferencesEditorTab tab);
		bool DeregisterTab(uint32_t tab);

	private:
		ApplicationState* appState = nullptr;
		Preferences* preferences = nullptr;

		std::unordered_map<uint32_t, PreferencesEditorTab> tabs;
		uint32_t selectedTab = 1;
	};

}