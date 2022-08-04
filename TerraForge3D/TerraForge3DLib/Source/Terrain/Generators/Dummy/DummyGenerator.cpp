#pragma once
#include "Terrain/Generators/Dummy/Generator.hpp"
#include "TerraForge3D.hpp"

namespace TerraForge3D
{
	namespace Terrain
	{

		void Dummy_Generator::OnAttach()
		{
			this->name = "Dummy Generator";
			this->editor = new Dummy_Editor(appState, &data);
			this->editor->SetVisible(false);
			this->cpuProcessor = new Dummy_CPUProcessor(&data);
		}

		void Dummy_Generator::OnDetach()
		{

		}
	}
}
