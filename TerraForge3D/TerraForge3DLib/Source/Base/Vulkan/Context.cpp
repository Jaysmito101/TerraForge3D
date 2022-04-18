#include "Base/Vulkan/Context.hpp"

#include "GLFW/glfw3.h"

namespace TerraForge3D
{
    namespace Vulkan
    {
        // The main vulkan context
        Context* Context::mainInstance = nullptr;

        Context::Context()
        {
            mainInstance = this;

            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "TerraForge3D";
            appInfo.applicationVersion = VK_MAKE_VERSION(TF3D_GENERATION, TF3D_VERSION_MAX, TF3D_VERSION_MIN);
            appInfo.pEngineName = "No Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_1;

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            createInfo.enabledExtensionCount = glfwExtensionCount;
            createInfo.ppEnabledExtensionNames = glfwExtensions;
            createInfo.enabledLayerCount = 0;
            TF3D_VK_CALL(vkCreateInstance(&createInfo, nullptr, &instance));

            SelectPhysicalDevices();
        }

        Context::~Context()
        {
            vkDestroyInstance(instance, nullptr);
            mainInstance = nullptr;
        }

        /*
        * This function selects the physical device to use for rendering and compute
        *
        * TerraForge3D supports multi GPU systems, so we need to select the physical device to use
        * one physical device is to be used for rendering the UI, the scenes, ...
        * and another one will be used for Compute purposes
        * 
        * If more than one GPU is not avialable then we will use the first one for both
        */
        void Context::SelectPhysicalDevices()
        {
            uint32_t physicalDeviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
            TF3D_ASSERT(physicalDeviceCount > 0, "No physical device found");
            if (physicalDeviceCount == 0)
            {
                TF3D_LOG_ERROR("No Vulkan compitable GPU found");
            }
        
            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

            TF3D_LOG("Found {0} Vulkan compitable GPU devices", physicalDeviceCount);

            std::vector<PhysicalDevice> physicalDeviceInfos(physicalDeviceCount);
            for(const auto& physicalDevice : physicalDevices)
            {
                physicalDeviceInfos.emplace_back(physicalDevice);
                // Debug
                TF3D_LOG("Device Info:- \n{0}", physicalDeviceInfos.back().ToString());
            }

            // TODO
        }

    }
}