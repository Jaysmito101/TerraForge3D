#pragma once

#include "Base/Vulkan/Core.hpp"
#include "Base/Vulkan/PhysicalDevice.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"
#include "Base/Vulkan/ComputeDevice.hpp"
#include "Base/Window/Window.hpp"

namespace TerraForge3D
{
    namespace Vulkan
    {
        
        class Context
        {
        private:
            Context();
            ~Context();

        private:
            std::vector<const char*> GetRequiredExtensions();

        public:

            inline static Context* Create() { TF3D_ASSERT(mainInstance == nullptr, "Vulkan context already exists"); mainInstance = new Context(); return mainInstance; } 
            inline static Context* Get() { TF3D_ASSERT(mainInstance, "No instance created"); return mainInstance; };
            inline static void Set(Context* context) { TF3D_ASSERT(context, "Context in NULL"); mainInstance = context; };
            inline static void Destroy() { TF3D_ASSERT(mainInstance, "No instance created");  TF3D_SAFE_DELETE(mainInstance); }

        private:
            void SelectPhysicalDevices();

        public:
            VkInstance instance = VK_NULL_HANDLE;
            GraphicsDevice* graphicsDevice = nullptr;
            ComputeDevice* computeDevice = nullptr;

            static Context* mainInstance;
        };
    }
}