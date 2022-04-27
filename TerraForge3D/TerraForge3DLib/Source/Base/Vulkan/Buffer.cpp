#include "Base/Vulkan/Buffer.hpp"

#include "Base/Vulkan/GraphicsDevice.hpp"
#include "Base/Vulkan/ComputeDevice.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{

		Buffer::Buffer(bool isGD)
		{
			isGraphicsDevice = isGD;
			if (isGraphicsDevice)
			{
				graphicsDevice = GraphicsDevice::Get();
				device = graphicsDevice->handle;
			}
			else
			{
				computeDevice = ComputeDevice::Get();
				device = computeDevice->handle;
			}
		}

		Buffer::~Buffer()
		{
			if (autoDestroy && isSetupOnGPU)
				Destory();
		}

		void Buffer::Setup()
		{
			TF3D_ASSERT(!isSetupOnGPU, "Buffer has already been setup (Call Destroy first)");
			TF3D_ASSERT(bufferSize != 0, "Buffer size cannot be zero");

			VkBufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = bufferSize;
			bufferCreateInfo.usage = usageflags;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			TF3D_VK_CALL(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &handle));

			VkMemoryAllocateInfo memoryAllocateInfo = {};
			memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memoryRequirements = {};
			// Get the memory requirements
			vkGetBufferMemoryRequirements(device, handle, &memoryRequirements);
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, propertyflags);
			TF3D_VK_CALL(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &deviceMemory));
			TF3D_VK_CALL(vkBindBufferMemory(device, handle, deviceMemory, 0));
			isSetupOnGPU = true;
		}

		void Buffer::Destory()
		{
			TF3D_ASSERT(isSetupOnGPU, "Buffer not yet setup");
			vkDestroyBuffer(device, handle, nullptr);
			vkFreeMemory(device, deviceMemory, nullptr);
			isSetupOnGPU = false;
		}

		void Buffer::Map(VkDeviceSize size, VkDeviceSize offset)
		{
			TF3D_ASSERT(isSetupOnGPU, "Buffer not yet setup");
			vkMapMemory(device, deviceMemory, offset, size, 0, &mapped);
		}

		void Buffer::Unmap()
		{
			TF3D_ASSERT(isSetupOnGPU, "Buffer not yet setup");
			TF3D_ASSERT(mapped, "Memory is not mapped yet");
			if (mapped)
			{
				vkUnmapMemory(device, deviceMemory);
				mapped = nullptr;
			}
		}

		void Buffer::Bind(VkDeviceSize offset)
		{
			TF3D_ASSERT(isSetupOnGPU, "Buffer not yet setup");
			TF3D_VK_CALL(vkBindBufferMemory(device, handle, deviceMemory, offset));
		}

		void Buffer::SetupDescriptor(VkDeviceSize size, VkDeviceSize offset)
		{
			TF3D_ASSERT(isSetupOnGPU, "Buffer not yet setup");
			descriptor.offset = offset;
			descriptor.buffer = handle;
			descriptor.range = size;
		}

		void Buffer::SetData(void* data, VkDeviceSize size)
		{
			TF3D_ASSERT(isSetupOnGPU, "Buffer not yet setup");
			TF3D_ASSERT(mapped, "Memory is not mapped yet");
			memcpy(mapped, data, size);
		}

		void Buffer::Flush(VkDeviceSize size, VkDeviceSize offset)
		{
			TF3D_ASSERT(isSetupOnGPU, "Buffer not yet setup");
			VkMappedMemoryRange mappedRange = {};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = deviceMemory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			TF3D_VK_CALL(vkFlushMappedMemoryRanges(device, 1, &mappedRange));
		}

		void Buffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
		{
			TF3D_ASSERT(isSetupOnGPU, "Buffer not yet setup");
			VkMappedMemoryRange mappedRange = {};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = deviceMemory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			TF3D_VK_CALL(vkInvalidateMappedMemoryRanges(device, 1, &mappedRange));
		}

		uint32_t Buffer::GetMemoryTypeIndex(uint32_t type, VkMemoryPropertyFlags properties)
		{
			if (isGraphicsDevice)
				return graphicsDevice->physicalDevice.GetMemoryTypeIndex(type, properties);
			else
				return computeDevice->physicalDevice.GetMemoryTypeIndex(type, properties);
		}

	}

}

#endif