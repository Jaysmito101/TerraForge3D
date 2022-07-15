#include "Base/Vulkan/LogicalDevice.hpp"

namespace TerraForge3D
{

	namespace Vulkan
	{

		LogicalDevice::LogicalDevice(PhysicalDevice& pd)
		{
			physicalDevice = pd;
		}
		
		void LogicalDevice::Setup()
		{
			CreateDevice();
			CreateDescriptorPool();
			CreateCommandPool();
		}

		LogicalDevice::~LogicalDevice()
		{
			vkDestroyDescriptorPool(handle, descriptorPool, nullptr);
			vkDestroyDevice(handle, nullptr);
		}

		VkCommandBuffer LogicalDevice::CreateCommandBuffer(VkCommandBufferLevel level, bool begin)
		{
			VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
			cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufferAllocateInfo.level = level;
			cmdBufferAllocateInfo.commandPool = commandPool;
			cmdBufferAllocateInfo.commandBufferCount = 1;
			cmdBufferAllocateInfo.pNext = nullptr;
			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			TF3D_VK_CALL(vkAllocateCommandBuffers(handle, &cmdBufferAllocateInfo, &commandBuffer));

			if (begin)
			{
				VkCommandBufferBeginInfo cmdBufBeginInfo = {};
				cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				TF3D_VK_CALL(vkBeginCommandBuffer(commandBuffer, &cmdBufBeginInfo));
			}
			return commandBuffer;
		}

		void LogicalDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer, bool free)
		{
			TF3D_ASSERT(commandBuffer != VK_NULL_HANDLE, "Command buffer is null");
			TF3D_VK_CALL(vkEndCommandBuffer(commandBuffer));

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			// Create Fence to ensure that the command buffer has finished executing
			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			// fenceInfo.flags = VK_FLAGS_NONE;
			VkFence fence;
			TF3D_VK_CALL(vkCreateFence(handle, &fenceInfo, nullptr, &fence));
			// Submit to the queue
			TF3D_VK_CALL(vkQueueSubmit(primaryQueue, 1, &submitInfo, fence));
			// Wait for the fence to signal that command buffer has finished executing
			TF3D_VK_CALL(vkWaitForFences(handle, 1, &fence, VK_TRUE, UINT64_MAX));
			vkDestroyFence(handle, fence, nullptr);

			if (free)
			{
				vkFreeCommandBuffers(handle, commandPool, 1, &commandBuffer);
			}


		}

		void LogicalDevice::CreateDescriptorPool()
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
		}

		void LogicalDevice::CreateCommandPool()
		{
			VkCommandPoolCreateInfo cmdPoolInfo = { };
			cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolInfo.queueFamilyIndex = primaryQueueProperties.index;
			cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			TF3D_VK_CALL(vkCreateCommandPool(handle, &cmdPoolInfo, nullptr, &commandPool));
		}


	}

}