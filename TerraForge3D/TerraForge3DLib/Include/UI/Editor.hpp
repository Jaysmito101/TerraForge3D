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
			~Editor();

			virtual void OnUpdate() = 0;
			virtual void OnShow() = 0;
			virtual void OnStart() = 0;
			virtual void OnEnd() = 0;

			void Update();
			void Show();
			void Setup();
			void Shutdown();


			inline Editor* SetEnabled(bool enabled) { this->isEnabled = enabled; return this;}
			inline Editor* SetName(std::string name) { this->name = name; return this;}
			inline Editor* ClearFlags() { this->windowFlags = ImGuiWindowFlags_None; return this;}
			inline Editor* AddFlags(ImGuiWindowFlags flags) { this->windowFlags |= flags; return this; }

			inline bool IsEnabled() { return this->isEnabled; }
			inline bool IsVisible() { return this->isVisible; }
			inline std::string GetName() { return this->name; }
			inline EditorManager* GetSubEditorManager() { return this->subEditorManager; }
			inline Menu* GetMenu() { return this->menu; }

		protected:
			std::string name = "Editor";
			UUID uid;
			std::string uidStr = "";
			EditorManager* subEditorManager = nullptr;
			bool isVisible = true;
			bool isEnabled = true;
			ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
			Menu* menu = nullptr;
		};

	}

}
