#pragma once
#include "Base/Base.hpp"
#include "Terrain/Processor.hpp"

namespace TerraForge3D
{
	class ApplicationState;
	namespace UI { class Editor; }

	namespace Terrain
	{

		class Generator
		{
		public:
			Generator(ApplicationState* appState)
				:appState(appState) 
			{}

			virtual ~Generator() = default;

			virtual void OnAttach() = 0;
			virtual void OnDetach() = 0;

		protected:
			ApplicationState* appState;

		public:
			std::string name = "Generator";
			std::string description = "";
			SharedPtr<UI::Editor> editor;
			SharedPtr<Processor> cpuProcessor;
#ifdef TF3D_VULKAN_BACKEND
			SharedPtr<Processor> vulkanProcessor;
#endif
			SharedPtr<Processor> openCLProcessor;
		};

	}
}
