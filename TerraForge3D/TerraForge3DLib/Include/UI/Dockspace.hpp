#pragma once
#include "Base/Core/Core.hpp"
#include "imgui/imgui.h"

namespace TerraForge3D
{

	namespace UI
	{

		class Dockspace
		{
		public:
			Dockspace() = default;
			~Dockspace() = default;

			void Begin();
			void End();

			inline void SetOpen(bool open) { this->isOpen = open; }
			inline bool IsOpen() { return this->isOpen; }

		private:
			bool isOpen = true;
			bool optFullScreenPersistant = true;
			bool optFullScreen = optFullScreenPersistant;
			ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
			ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
			UUID uid = UUID::Generate();
		};

	}

}
