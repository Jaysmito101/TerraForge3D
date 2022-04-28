#include "Base/Vulkan/GraphicsDevice.hpp"
#include "Base/Vulkan/ValidationLayers.hpp"
#include "Base/Vulkan/SwapChain.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{
	namespace Vulkan
	{
#ifdef TF3D_DEBUG
		extern bool g_ValidationLayersSupported;
#endif
		GraphicsDevice* GraphicsDevice::mainInstance = nullptr;

		GraphicsDevice::GraphicsDevice(PhysicalDevice& pD)
			:LogicalDevice(pD)
		{
			extensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
			physicalDevice = pD;
			TF3D_LOG("Using Graphics Device: {0}", physicalDevice.name);
			CreateDevice();
			CreateDescriptorPool();
		}

		GraphicsDevice::~GraphicsDevice()
		{
		}

		void GraphicsDevice::CreateDevice()
		{
			if (!physicalDevice.isGraphicsCapable)
			{
				TF3D_LOG_ERROR("Vulkan device is not graphics compatible");
				exit(-1);
			}

			uint32_t graphicsQueueFamilyIndex = physicalDevice.GetGraphicsQueueIndex();
			TF3D_ASSERT(graphicsQueueFamilyIndex >= 0, "Invalid queue index");
			graphicsQueueProperties = physicalDevice.GetQueue(graphicsQueueFamilyIndex);
			uint32_t presentQueueFamilyIndex = physicalDevice.GetPresentQueueIndex(SwapChain::Get()->GetSurface());
			TF3D_ASSERT(presentQueueFamilyIndex >= 0, "Invalid queue index");
			presentQueueProperties = physicalDevice.GetQueue(presentQueueFamilyIndex);

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
			
			createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
			createInfo.ppEnabledExtensionNames = extensions.data();

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
			
			TF3D_VK_CALL(vkCreateDevice(physicalDevice.handle, &createInfo, nullptr, &handle));

			vkGetDeviceQueue(handle, graphicsQueueFamilyIndex, 0, &graphicsQueue);
			vkGetDeviceQueue(handle, presentQueueFamilyIndex, 0, &presentQueue);

			primaryQueue = graphicsQueue;
			primaryQueueProperties = graphicsQueueProperties;
		}


	}
}

#endif