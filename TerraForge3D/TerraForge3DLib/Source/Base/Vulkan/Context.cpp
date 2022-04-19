#include "Base/Vulkan/Context.hpp"
#include "Base/Vulkan/ValidationLayers.hpp"

#include "GLFW/glfw3.h"

namespace TerraForge3D
{
    namespace Vulkan
    {
#ifdef TF3D_DEBUG
        static bool g_ValidationLayersSupported = false;
#endif

        // The main vulkan context
        Context* Context::mainInstance = nullptr;

        Context::Context()
        {
            mainInstance = this;

#ifdef TF3D_DEBUG
            g_ValidationLayersSupported = ValidationLayers::CheckSupport();
            if (!g_ValidationLayersSupported)
                TF3D_LOG_ERROR("Validation layers are not supported")
#endif

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

            auto requiredExtensions = GetRequiredExtensions();
            createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
            createInfo.ppEnabledExtensionNames = requiredExtensions.data();

#ifdef TF3D_DEBUG
            if (g_ValidationLayersSupported)
            {
                createInfo.enabledLayerCount = ValidationLayers::GetLayerCount();
                createInfo.ppEnabledLayerNames = ValidationLayers::GetLayerNames();
                VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
                createInfo.pNext = ValidationLayers::PopulateDebugMessengerCreateInfo(&debugCreateInfo);
            }
            else
            {
                createInfo.enabledLayerCount = 0;
                createInfo.pNext = nullptr;
            }
#else
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
#endif

            TF3D_VK_CALL(vkCreateInstance(&createInfo, nullptr, &instance));
            TF3D_LOG("Vulkan instance created")

#ifdef  TF3D_DEBUG
                if (g_ValidationLayersSupported)
                {
                    ValidationLayers::CreateDebugMessenger(instance);
                    TF3D_LOG("Vulkan Debug messenger setup")
                }
#endif //  TF3D_DEBUG


            SelectPhysicalDevices();
        }

        Context::~Context()
        {
#ifdef TF3D_DEBUG
            if (g_ValidationLayersSupported)
                ValidationLayers::DestroyDebugMessenger(instance);
#endif
            vkDestroyInstance(instance, nullptr);
            mainInstance = nullptr;
        }

        std::vector<const char*> Context::GetRequiredExtensions()
        {
            // Get the required extensions from GLFW
            uint32_t glfwExtensionsCount = 0;
            const char** glfwExtensions = nullptr;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
            std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);

#ifdef TF3D_DEBUG
            // In debug builds we also need the DEBUG_UTILS_EXTENSION for debug messages
            if (g_ValidationLayersSupported)
                requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
            return requiredExtensions;
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