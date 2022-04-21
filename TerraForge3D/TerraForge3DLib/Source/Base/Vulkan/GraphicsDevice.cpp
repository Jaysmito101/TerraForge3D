#include "Base/Vulkan/GraphicsDevice.hpp"
#include "Base/Vulkan/ValidationLayers.hpp"
#include "Base/Vulkan/SwapChain.hpp"

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
			uint32_t graphicsQueueFamilyIndex = physicalDevice.GetGraphicsQueueIndex();
			TF3D_ASSERT(graphicsQueueFamilyIndex >= 0, "Invalid queue index");
			uint32_t presentQueueFamilyIndex = physicalDevice.GetPresentQueueIndex(SwapChain::Get()->GetSurface());
			TF3D_ASSERT(presentQueueFamilyIndex >= 0, "Invalid queue index");

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

			std::set<uint32_t> uniqueQueueFamilies = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};

			
			float queuePriority = 1.0f;
			for (uint32_t queueFamily : uniqueQueueFamilies) {
				VkDeviceQueueCreateInfo queueCreateInfo{};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkPhysicalDeviceFeatures deviceFeatures{};

			VkDeviceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
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

			vkGetDeviceQueue(handle, graphicsQueueFamilyIndex, 0, &graphicsQueue);
			vkGetDeviceQueue(handle, presentQueueFamilyIndex, 0, &presentQueue);
		}


	}
}