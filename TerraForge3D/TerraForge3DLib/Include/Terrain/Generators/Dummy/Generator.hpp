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

			struct Dummy_SharedData
			{

			};

			class Dummy_Generator : public Terrain::Generator
			{
			public:
				Dummy_Generator(ApplicationState* appState)
					:Terrain::Generator(appState)
				{}

				virtual ~Dummy_Generator() = default;

				virtual void OnAttach() override;
				virtual void OnDetach() override;

			public:
				Dummy_SharedData data;				
			};
	}
}
