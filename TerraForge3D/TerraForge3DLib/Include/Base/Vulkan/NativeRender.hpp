#pragma once
#include "Base/Vulkan/Core.hpp"
#include "Base/Renderer/NativeRenderer.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{
		class Context;
		class GraphicsDevice;

		class NativeRenderer : public RendererAPI::NativeRenderer
		{
		public:
			NativeRenderer();
			~NativeRenderer();

			virtual void Flush() override;

		private:
			Context* context = nullptr;
			GraphicsDevice* device = nullptr;
		};

	}

}
#endif