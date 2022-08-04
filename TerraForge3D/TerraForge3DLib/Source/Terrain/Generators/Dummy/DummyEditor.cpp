#include "Terrain/Generators/Dummy/Generator.hpp"
#include "TerraForge3D.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{
		Dummy_Editor::Dummy_Editor(ApplicationState* as, Dummy_SharedData* sd)
			:UI::Editor("Dummy Generator Editor"), appState(as), data(sd)
		{}

		void Dummy_Editor::OnUpdate()
		{

		}

		void Dummy_Editor::OnShow()
		{
			ImGui::Text("Dummy Editor");
		}

		void Dummy_Editor::OnStart()
		{

		}

		void Dummy_Editor::OnEnd()
		{

		}

	}

}
