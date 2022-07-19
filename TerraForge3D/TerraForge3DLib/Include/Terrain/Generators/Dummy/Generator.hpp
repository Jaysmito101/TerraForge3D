#pragma once
#include "Base/Base.hpp"
#include "Terrain/Generators/Generator.hpp"
#include "Terrain/Generators/Dummy/CPUProcessor.hpp"
#include "Terrain/Generators/Dummy/GPUProcessor.hpp"
#include "Terrain/Generators/Dummy/Editor.hpp"

namespace TerraForge3D
{
	namespace Terrain
	{
		namespace Dummy
		{
			class Generator : public Terrain::Generator
			{
			public:
				Generator(ApplicationState* appState)
					:Terrain::Generator(appState)
				{}

				virtual ~Generator() = default;

				virtual void OnAttach() override;
				virtual void OnDetach() override;

			private:

			public:
				
			};
		}
	}
}
