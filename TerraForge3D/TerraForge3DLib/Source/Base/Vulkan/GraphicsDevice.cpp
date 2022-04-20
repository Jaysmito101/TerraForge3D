#include "Base/Vulkan/GraphicsDevice.hpp"
#include "Base/Vulkan/ValidationLayers.hpp"

namespace TerraForge3D
{
	namespace Vulkan
	{
#ifdef TF3D_DEBUG
		extern bool g_ValidationLayersSupported;
#endif
		GraphicsDevice* GraphicsDevice::mainInstance = nullptr;

		GraphicsDevice::GraphicsDevice(PhysicalDevice& pD)
		{
			physicalDevice = pD;
			if (!physicalDevice.isGraphicsCapable)
			{
				TF3D_LOG_ERROR("Vulkan device is not graphics compatible");
				exit(-1);
			}
			TF3D_LOG("Using Graphics Device: {0}", physicalDevice.name);
			CreateDevice();
		}

		GraphicsDevice::~GraphicsDevice()
		{
			vkDestroyDevice(handle, nullptr);
		}

		void GraphicsDevice::CreateDevice()
		{
			uint32_t queueFamilyIndex = physicalDevice.GetGraphicsQueueIndex();
			TF3D_ASSERT(queueFamilyIndex >= 0, "Invalid queue index");

			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
			queueCreateInfo.queueCount = 1;
			float queuePriority = 1.0f;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			VkPhysicalDeviceFeatures deviceFeatures{};

			VkDeviceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.pQueueCreateInfos = &queueCreateInfo;
			createInfo.queueCreateInfoCount = 1;
			createInfo.pEnabledFeatures = &deviceFeatures;
			createInfo.enabledExtensionCount = 0;
#ifdef TF3D_DEBUG
			if (g_ValidationLayersSupported) {
				createInfo.enabledLayerCount = ValidationLayers::GetLayerCount();
				createInfo.ppEnabledLayerNames = ValidationLayers::GetLayerNames();
			}
			else {
				createInfo.enabledLayerCount = 0;
			}
#else
			createInfo.enabledLayerCount = 0;
#endif
			
			TF3D_VK_CALL(vkCreateDevice(physicalDevice.handle, &createInfo, nullptr, &handle) != VK_SUCCESS);

			vkGetDeviceQueue(handle, queueFamilyIndex, 0, &queue);
		}


	}
}