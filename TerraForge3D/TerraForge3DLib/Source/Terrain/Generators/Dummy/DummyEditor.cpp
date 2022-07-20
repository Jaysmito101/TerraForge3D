#include "Terrain/Generators/Dummy/Generator.hpp"
#include "TerraForge3D.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{
		namespace Dummy
		{
			Editor::Editor(ApplicationState* as, SharedData* sd)
				:UI::Editor("Dummy Generator Editor"), appState(as), data(sd)
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
