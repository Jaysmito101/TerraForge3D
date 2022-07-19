#pragma once

#include "Base/OpenGL/Core.hpp"
#include "Base/Renderer/Context.hpp"

namespace TerraForge3D
{

	namespace OpenGL
	{
	
		class Context : public RendererAPI::Context
		{
		public:
			Context();
			~Context();

			std::string GetGPUName();

			void WaitForIdle() override;
		};

	}

}