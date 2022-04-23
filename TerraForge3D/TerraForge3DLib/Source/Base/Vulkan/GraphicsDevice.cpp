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
			extensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
			physicalDevice = pD;
			if (!physicalDevice.isGraphicsCapable)
			{
				TF3D_LOG_ERROR("Vulkan device is not graphics compatible");
				exit(-1);
			}
			TF3D_LOG("Using Graphics Device: {0}", physicalDevice.name);
			CreateDevice();
			CreateDescriptorPool();
		}

		GraphicsDevice::~GraphicsDevice()
		{
			vkDestroyDescriptorPool(handle, descriptorPool, nullptr);
			vkDestroyDevice(handle, nullptr);
		}

		void GraphicsDevice::CreateDevice()
		{
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
		}

		void GraphicsDevice::CreateDescriptorPool()
		{
			VkDescriptorPoolSize poolSizes[] = 
			{
					{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
					{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
					{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
					{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo poolCreateInfo = {};
			poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			poolCreateInfo.maxSets = 1000 * sizeof(poolSizes) / sizeof(poolSizes[0]);
			poolCreateInfo.poolSizeCount = static_cast<uint32_t>(sizeof(poolSizes) / sizeof(poolSizes[0]));
			poolCreateInfo.pPoolSizes = poolSizes;
			TF3D_VK_CALL(vkCreateDescriptorPool(handle, &poolCreateInfo, nullptr, &descriptorPool));
			TF3D_LOG("Created descriptor pool for graphics device");
		}

	}
}