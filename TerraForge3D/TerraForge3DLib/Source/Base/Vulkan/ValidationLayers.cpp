#include "Base/Vulkan/ValidationLayers.hpp"

#ifdef TF3D_VULKAN_BACKEND

#ifdef TF3D_DEBUG // Enable Validation Layers only in Debug builds

static std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        TF3D_LOG_INFO("[Vulkan]: {0}", pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        TF3D_LOG_TRACE("[Vulkan]: {0}", pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        TF3D_LOG_WARN("[Vulkan]: {0}", pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        TF3D_LOG_ERROR("[Vulkan]: {0}", pCallbackData->pMessage);
    }
    return VK_TRUE;
}

namespace TerraForge3D
{
	namespace Vulkan
	{
        VkDebugUtilsMessengerEXT ValidationLayers::debugMessenger;

		bool ValidationLayers::CheckSupport()
		{
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            for (const char* layerName : validationLayers) {
                bool layerFound = false;

                for (const auto& layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        TF3D_LOG("Validation layer {0} available", layerProperties.layerName)
                        break;
                    }
                }

                if (!layerFound) {
                    TF3D_LOG_ERROR("Validation layer {0} not avaliable", layerName)
                    return false;
                }
            }
			return true;
		}

        uint32_t ValidationLayers::GetLayerCount()
        {
            return static_cast<uint32_t>(validationLayers.size());
        }

        const char** ValidationLayers::GetLayerNames()
        {
            return validationLayers.data();
        }

        VkDebugUtilsMessengerCreateInfoEXT* ValidationLayers::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo)
        {
            createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo->pfnUserCallback = VulkanDebugCallback;
            return createInfo;
        }

        void ValidationLayers::CreateDebugMessenger(VkInstance instance)
        {
            VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
            PopulateDebugMessengerCreateInfo(&createInfo);

            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func == nullptr)
            {
                TF3D_LOG_ERROR("vkCreateDebugUtilsMessengerEXT could not be loaded");
                exit(-1);
            }
            func(instance, &createInfo, nullptr, &debugMessenger);
        }

        void ValidationLayers::DestroyDebugMessenger(VkInstance instance)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func == nullptr)
                TF3D_LOG_ERROR("vkDestroyDebugUtilsMessengerEXT could not be loaded")
            else
                func(instance, debugMessenger, nullptr);
        }
	}
}

#endif // TF3D_DEBUG

#endif // TF3D_VULKAN_BACKEND