#pragma once

#include "Base/Vulkan/Core.hpp"
#include "Base/Vulkan/PhysicalDevice.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"
#include "Base/Vulkan/ComputeDevice.hpp"
#include "Base/Vulkan/SwapChain.hpp"
#include "Base/Window/Window.hpp"
#include "Base/Renderer/Context.hpp"

namespace TerraForge3D
{
    namespace Vulkan
    {
        
        class Context : public RendererAPI::Context
        {
        public:
            Context();
            ~Context();

            void WaitForIdle() override;

        private:
            std::vector<const char*> GetRequiredExtensions();

            void SelectPhysicalDevices();

        public:
            VkInstance instance = VK_NULL_HANDLE;
            GraphicsDevice* graphicsDevice = nullptr;
            ComputeDevice* computeDevice = nullptr;
            SwapChain* swapChain = nullptr;
        };
    }
}