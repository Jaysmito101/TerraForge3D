#pragma once
#include "Base/Vulkan/Core.hpp"

namespace TerraForge3D
{

	namespace Vulkan
	{

		class GraphicsDevice;
		class ComputeDevice;

		enum BufferUsage
		{
			BufferUsage_TransferSource = 0x00000001,
			BufferUsage_TransferDestination = 0x00000002,
			BufferUsage_UniformTexelBuffer = 0x00000004,
			BufferUsage_StorageTexelBuffer = 0x00000008,
			BufferUsage_UniformBuffer = 0x00000010,
			BufferUsage_StorageBuffer = 0x00000020,
			BufferUsage_IndexBuffer = 0x00000040,
			BufferUsage_VertexBuffer = 0x00000080,
			BufferUsage_IndirectBuffer = 0x00000100,
			BufferUsage_ShaderDeviceAddressBuffer = 0x00020000,
			BufferUsage_ShaderBindingTable = 0x00000400,
			BufferUsage_RayTracing = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
			BufferUsage_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
		};

		class Buffer 
		{
		public:
			Buffer(bool isGraphicsDevice = true);
			~Buffer();

			void Setup();
			void Destory();

			void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
			void Unmap();
			void Bind(VkDeviceSize offset = 0);
			void SetupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
			void SetData(void* data, VkDeviceSize size);
			void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
			void Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
			uint32_t GetMemoryTypeIndex(uint32_t type, VkMemoryPropertyFlags properties);

			inline void SetUsageFlags(VkBufferUsageFlags flags) { usageflags = flags; };
			inline void SetMemoryProperties(VkMemoryPropertyFlags flags) { propertyflags = flags; };

		public:
			VkBuffer handle = VK_NULL_HANDLE;
			VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
			VkDevice device = VK_NULL_HANDLE;
			VkDeviceSize bufferSize = 0;
			VkDeviceSize alingment = 0;
			VkMemoryPropertyFlags propertyflags;
			VkBufferUsageFlags usageflags;
			VkDescriptorBufferInfo descriptor;

			GraphicsDevice* graphicsDevice = nullptr;
			ComputeDevice* computeDevice = nullptr;


			void* mapped = nullptr;
			bool autoDestroy = true;
			bool isGraphicsDevice = true;
			bool isSetupOnGPU = false;
		};

	}

}
