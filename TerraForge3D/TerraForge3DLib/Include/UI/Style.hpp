#pragma once
#include "Base/Core/Core.hpp"

#include "imgui/imgui.h"

namespace TerraForge3D
{

	namespace UI
	{

		class Style
		{
		public:
			Style();
			~Style();

			void LoadFromFile(std::string filepath);
			void LoadFormString(std::string contents);
			void LoadCurrent();

			void SaveToFile(std::string filepath);
			std::string SaveToString();
			void Apply();

			inline void SetName(std::string name) { this->name = name; }
			inline std::string GetName() { return this->name; }

			inline void SetStyle(ImGuiStyle style) { this->style = style; }
			inline ImGuiStyle GetStyle() { return this->style; }

		private:
			ImGuiStyle style;
			std::string name = "Style";
		};

	}

}
