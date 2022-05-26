#include "Base/Vulkan/NativeRender.hpp"
#include "Base/Vulkan/Context.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{

		NativeRenderer::NativeRenderer()
		{
			this->context = reinterpret_cast<Context*>(Context::Get());
			this->device = GraphicsDevice::Get();
		}

		NativeRenderer::~NativeRenderer()
		{
		}

		void NativeRenderer::Flush()
		{
			
		}

	}

}

#endif