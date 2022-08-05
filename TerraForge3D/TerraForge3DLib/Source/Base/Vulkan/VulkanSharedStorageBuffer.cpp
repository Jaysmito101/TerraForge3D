#include "Base/Vulkan/SharedStorageBuffer.hpp"
#include "Base/Vulkan/ComputeDevice.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"
#include "Base/Vulkan/Buffer.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{

		SharedStorageBuffer::SharedStorageBuffer()
		{
		}

		SharedStorageBuffer::~SharedStorageBuffer()
		{
			if (this->autoDestory && this->isSetup)
				this->Destroy();
		}

		bool SharedStorageBuffer::Setup() 
		{
			TF3D_ASSERT(!isSetup, "Buffer has already been setup");
			TF3D_ASSERT(size > 0, "Buffer size cannot be <= 0");

			LogicalDevice* device = nullptr;
			if (this->isGraphicsUse)
				device = GraphicsDevice::Get();
			else
				device = ComputeDevice::Get();
			
			TF3D_ASSERT(device, "Logical device handle is NULL");

			buffer = new Buffer();
			buffer->bufferSize = this->size;
			buffer->isGraphicsDevice = isGraphicsUse;
			buffer->usageflags = BufferUsage_StorageBuffer;
			buffer->Setup();

			VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
			descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			// descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			descriptorSetLayoutBinding.binding = this->binding;
			descriptorSetLayoutBinding.descriptorCount = 1;
			descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.bindingCount = 1;
			descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;
			descriptorSetLayoutCreateInfo.flags = 0;
			descriptorSetLayoutCreateInfo.pNext = nullptr;

			TF3D_VK_CALL(vkCreateDescriptorSetLayout(device->handle, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));

			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = device->descriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
			descriptorSetAllocateInfo.pNext = nullptr;

			TF3D_VK_CALL(vkAllocateDescriptorSets(device->handle, &descriptorSetAllocateInfo, &descriptorSet));

			VkDescriptorBufferInfo descriptorBufferInfo{};
			descriptorBufferInfo.buffer = buffer->handle;
			descriptorBufferInfo.offset = 0;
			descriptorBufferInfo.range = buffer->bufferSize;


			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			// writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			writeDescriptorSet.dstSet = descriptorSet;
			writeDescriptorSet.dstBinding = this->binding;
			writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
			writeDescriptorSet.pNext = nullptr;

			vkUpdateDescriptorSets(device->handle, 1, &writeDescriptorSet, 0, nullptr);
			
			isSetup = true;

			return true;
		}

		bool SharedStorageBuffer::Destroy()
		{
			TF3D_ASSERT(isSetup, "Buffer has not yet been setup");

			LogicalDevice* device = nullptr;
			if (this->isGraphicsUse)
				device = GraphicsDevice::Get();
			else
				device = ComputeDevice::Get();

			TF3D_ASSERT(device, "Logical device handle is NULL");

			vkDeviceWaitIdle(device->handle);

			vkDestroyDescriptorSetLayout(device->handle, descriptorSetLayout, nullptr);

			vkFreeDescriptorSets(device->handle, device->descriptorPool, 1, &descriptorSet);

			buffer->Destory();
			TF3D_SAFE_DELETE(buffer);

			isSetup = false;

			return true;
		}

		bool SharedStorageBuffer::SetData(void* data, uint64_t lSize, uint64_t lOffset)
		{
			TF3D_ASSERT(isSetup, "Buffer has not yet been setup");
			TF3D_ASSERT(lSize <= this->size, "Invalid size");
			TF3D_ASSERT(lOffset >= 0, "Invalid offset");
			TF3D_ASSERT((lOffset + lSize) <= this->size, "Invalid offset");

			buffer->Map();
			buffer->SetData(data, lSize, lOffset);
			buffer->Unmap();
			return true;
		}

		bool SharedStorageBuffer::GetData(void* data, uint64_t lSize, uint64_t lOffset)
		{
			TF3D_ASSERT(isSetup, "Buffer has not yet been setup");
			TF3D_ASSERT(lSize <= this->size, "Invalid size");
			TF3D_ASSERT(lOffset >= 0, "Invalid offset");
			TF3D_ASSERT((lOffset + lSize) <= this->size, "Invalid offset");

			buffer->Map();
			buffer->GetData(data, lSize, lOffset);
			buffer->Unmap();
			return true;
		}

	}

}

#endif // TF3D_VULKAN_BACKEND