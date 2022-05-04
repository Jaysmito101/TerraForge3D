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

		private:
			ImGuiStyle style;
		};

	}

}
