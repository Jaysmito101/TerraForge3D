#pragma once

#include "Base/Vulkan/Core.hpp"
#include "Base/Renderer/SharedStorageBuffer.hpp"

#ifdef TF3D_VULKAN_BACKEND


namespace TerraForge3D
{

	namespace Vulkan
	{
		class Buffer;

		class SharedStorageBuffer : public RendererAPI::SharedStorageBuffer
		{
		public:
			SharedStorageBuffer();
			~SharedStorageBuffer();

			bool Setup();
			bool Destroy();
			bool SetData(void* data, uint64_t size, uint64_t offset = 0);
			bool GetData(void* data, uint64_t size, uint64_t offset = 0);

		public:
			Buffer* buffer = nullptr;
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		};

	}

}

#endif