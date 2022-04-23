#include "Base/Vulkan/SwapChain.hpp"
#include "Base/Window/Window.hpp"
#include "Base/Vulkan/Context.hpp"

#include "GLFW/glfw3.h"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{
	namespace Vulkan
	{

		SwapChain* SwapChain::mainInstance;

		SwapChain::SwapChain(Vulkan::Context* context)
		{
			mainWindow = Window::Get()->GetHandle();
			instance = context->instance;

			TF3D_VK_CALL(glfwCreateWindowSurface(instance, mainWindow, nullptr, &surface));
			TF3D_LOG("Created window surface");
		}

		SwapChain::~SwapChain()
		{
			vkDestroySurfaceKHR(instance, surface, nullptr);
		}


	}
}

#endif