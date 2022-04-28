#pragma once
#include "Base/Vulkan/Core.hpp"
#include "Base/Vulkan/PhysicalDevice.hpp"

namespace TerraForge3D
{
	namespace Vulkan
	{

		class LogicalDevice
		{
		public:
			LogicalDevice(PhysicalDevice& physicalDevice);
			~LogicalDevice();

			void Setup();

			VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin = true);
			void FlushCommandBuffer(VkCommandBuffer commandBuffer, bool free = true);


			virtual void CreateDevice() = 0;

			void CreateDescriptorPool();
			void CreateCommandPool();

			PhysicalDevice physicalDevice;
			VkQueue primaryQueue = VK_NULL_HANDLE;
			VkDevice handle = VK_NULL_HANDLE;
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			VkCommandPool commandPool = VK_NULL_HANDLE;
			PhysicalDeviceQueueFamilyProperties primaryQueueProperties;
		};

	}

}
