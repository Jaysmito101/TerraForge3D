#pragma once
#include "Base/Core/Core.hpp"

#include "imgui/imgui.h"

namespace TerraForge3D
{
	namespace UI
	{
		class Menu;
		class EditorManager;

		class Editor
		{
		public:
			Editor(std::string name = "Editor");
			virtual ~Editor();

			virtual void OnUpdate() = 0;
			virtual void OnShow() = 0;
			virtual void OnStart() = 0;
			virtual void OnEnd() = 0;

			virtual bool OnContextMenu();

			void Update();
			void Show();
			void Setup();
			void Shutdown();


			inline Editor* SetEnabled(bool enabled) { this->isEnabled = enabled; return this; }
			inline Editor* SetVisible(bool enabled) { this->isVisible = enabled; return this;}
			inline Editor* SetName(std::string name) { this->name = name; return this;}
			inline Editor* ClearFlags() { this->windowFlags = ImGuiWindowFlags_None; return this;}
			inline Editor* AddFlags(ImGuiWindowFlags flags) { this->windowFlags |= flags; return this; }

			inline bool IsEnabled() { return this->isEnabled; }
			inline bool IsVisible() { return this->isVisible; }
			inline bool IsHovered() { return this->state.isHovered; }
			inline bool IsFocused() { return this->state.isFocused; }
			inline std::string GetName() { return this->name; }
			inline SharedPtr<EditorManager> GetSubEditorManager() { return this->subEditorManager; }
			inline SharedPtr<Menu> GetMenu() { return this->menu; }

		public:
			struct
			{
				bool isHovered = false;
				bool isFocused = false;
			} state;
		protected:
			std::string name = "Editor";
			UUID uid;
			std::string uidStr = "";
			bool isVisible = true;
			bool isEnabled = true;
			ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
			SharedPtr<EditorManager> subEditorManager ;
			SharedPtr<Menu> menu;
		};

	}

}
