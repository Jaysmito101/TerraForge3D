#pragma once
#include "Base/Base.hpp"
#include "Terrain/Generators/Generator.hpp"
#include "Terrain/Generators/Dummy/CPUProcessor.hpp"
#include "Terrain/Generators/Dummy/VulkanProcessor.hpp"
#include "Terrain/Generators/Dummy/OpenCLProcessor.hpp"
#include "Terrain/Generators/Dummy/Editor.hpp"

namespace TerraForge3D
{
	namespace Terrain
	{
		namespace Dummy
		{

			struct SharedData
			{

			};

			class Generator : public Terrain::Generator
			{
			public:
				Generator(ApplicationState* appState)
					:Terrain::Generator(appState)
				{}

				virtual ~Generator() = default;

				virtual void OnAttach() override;
				virtual void OnDetach() override;

			public:
				SharedData data;
				
			};
		}
	}
}
