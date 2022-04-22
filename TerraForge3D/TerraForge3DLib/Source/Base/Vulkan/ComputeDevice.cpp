#include "Base/Vulkan/ComputeDevice.hpp"
#include "Base/Vulkan/ValidationLayers.hpp"

namespace TerraForge3D
{
	namespace Vulkan
	{
		ComputeDevice* ComputeDevice::mainInstance = nullptr;
#ifdef TF3D_DEBUG
		extern bool g_ValidationLayersSupported;
#endif

		ComputeDevice::ComputeDevice(PhysicalDevice& pD)
		{
			extensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
			physicalDevice = pD;
			if (!physicalDevice.isComputeCapable)
			{
				TF3D_LOG_ERROR("Vulkan device is not compute compatible");
				exit(-1);
			}
			TF3D_LOG("Using Compute Device: {0}", physicalDevice.name);
			CreateDevice();
			CreateDescriptorPool();
		}

		ComputeDevice::~ComputeDevice()
		{
			vkDestroyDescriptorPool(handle, descriptorPool, nullptr);
			vkDestroyDevice(handle, nullptr);
		}

		void ComputeDevice::CreateDevice()
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
			createInfo.enabledExtensionCount = extensions.size();
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

			TF3D_VK_CALL(vkCreateDevice(physicalDevice.handle, &createInfo, nullptr, &handle) != VK_SUCCESS);

			vkGetDeviceQueue(handle, queueFamilyIndex, 0, &queue);
		}

		void ComputeDevice::CreateDescriptorPool()
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
			TF3D_LOG("Created descriptor pool for compute device");
		}
	}
}