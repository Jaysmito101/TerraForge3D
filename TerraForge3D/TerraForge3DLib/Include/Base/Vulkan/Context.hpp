#pragma once

#include "Base/Vulkan/Core.hpp"
#include "Base/Vulkan/PhysicalDevice.hpp"
#include "Base/Window/Window.hpp"

namespace TerraForge3D
{
    namespace Vulkan
    {
        
        class Context
        {
        public:
            Context();
            ~Context();

        public:
            inline static Context* Get() { TF3D_ASSERT(mainInstance, "No instance created"); return mainInstance; };
            inline static void Set(Context* context) { TF3D_ASSERT(context, "Context in NULL"); mainInstance = context; };

        private:
            void SelectPhysicalDevices();

        public:
            VkInstance instance;
            

            static Context* mainInstance;
        };
    }
}