#pragma once
#include "Base/Vulkan/Core.hpp"

struct GLFWwindow;

namespace TerraForge3D
{
	namespace Vulkan
	{
		class GraphicsDevice;
		class Context;

		/*
		* This handles the main application SwapChain,
		* presenting the rendered image, ...
		*/
		class SwapChain
		{
		private:
			SwapChain(Vulkan::Context* context);
			~SwapChain();

		public:
			inline VkSurfaceKHR GetSurface() { return surface; }
			inline void SetGraphicsDevice(GraphicsDevice* gd) { graphicsDevice = gd; }
			
		public:
			inline static SwapChain* Create(Context* context) { TF3D_ASSERT(mainInstance == nullptr, "Swapchain already exists"); mainInstance = new SwapChain(context); return mainInstance; }
			inline static SwapChain* Get() { TF3D_ASSERT(mainInstance, "Swapchain not yet created"); return mainInstance; }
			inline static SwapChain* Set(SwapChain* swapChain) { TF3D_ASSERT(swapChain, "swapChain is null"); mainInstance = swapChain; return mainInstance; }
			inline static void Destroy() { TF3D_ASSERT(mainInstance, "Swapchain not yet created"); TF3D_SAFE_DELETE(mainInstance); }

		private:
			GLFWwindow* mainWindow = nullptr;
			GraphicsDevice* graphicsDevice = nullptr;
			VkInstance instance = VK_NULL_HANDLE;
			VkSurfaceKHR surface = VK_NULL_HANDLE;
			

			static SwapChain* mainInstance;

		};

	}
}
