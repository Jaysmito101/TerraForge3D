#pragma once
#include "Base/Vulkan/Core.hpp"
#include "Base/Renderer/Pipeline.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{

		class Pipeline : public RendererAPI::Pipeline
		{
		public:
			Pipeline();
			~Pipeline();

			virtual void Setup() override;
			virtual void Destory() override;

		public:

		};

	}

}

#endif