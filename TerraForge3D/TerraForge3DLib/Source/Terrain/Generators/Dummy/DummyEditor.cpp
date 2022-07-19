#pragma once
#include "Terrain/Generators/Dummy/Editor.hpp"
#include "TerraForge3D.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{
		namespace Dummy
		{
			Editor::Editor(ApplicationState* as)
				:UI::Editor("Dummy Generator Editor"), appState(as)
			{}

			void Editor::OnUpdate()
			{

			}
			
			void Editor::OnShow()
			{
				ImGui::Text("Dummy Editor");
			}
			
			void Editor::OnStart()
			{

			}

			void Editor::OnEnd()
			{

			}
		}
	}

}
