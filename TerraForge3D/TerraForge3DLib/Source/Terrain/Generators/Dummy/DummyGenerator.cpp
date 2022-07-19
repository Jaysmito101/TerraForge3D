#pragma once
#include "Terrain/Generators/Dummy/Generator.hpp"
#include "TerraForge3D.hpp"

namespace TerraForge3D
{
	namespace Terrain
	{

		void Dummy::Generator::OnAttach()
		{
			this->name = "Dummy Generator";
			this->editor = new Dummy::Editor(appState);
			this->editor->SetVisible(false);
		}

		void Dummy::Generator::OnDetach()
		{

		}
	}
}
