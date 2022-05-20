#pragma once
#include "Base/Renderer/Framebuffer.hpp"
#include "Base/Vulkan/Core.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{

		class FrameBuffer : public RendererAPI::FrameBuffer
		{
		public:
			FrameBuffer();
			~FrameBuffer();

			virtual void Setup() override;

		private:
		};

	}

}

#endif