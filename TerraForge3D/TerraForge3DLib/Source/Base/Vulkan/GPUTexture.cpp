#include "Base/Vulkan/GPUTexture.hpp"
#include "Base/Vulkan/ComputeDevice.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"

#include "imgui/backends/imgui_impl_vulkan.h"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{
		GPUTexture::GPUTexture()
		{
		}

		GPUTexture::~GPUTexture()
		{
			if (autoDestroy)
				Destroy();
		}

		void GPUTexture::Setup()
		{
			TF3D_ASSERT(!isSetupOnGPU, "Texture already setup on GPU (Call Destroy first)");
			TF3D_ASSERT(sizeX > 0 && sizeY > 0 && sizeZ > 0, "Texture sizes cannot be all 0");
			UpdateInfo();

			UpdateInfo();
			
			uint64_t size = sizeX * sizeY * sizeZ * bytesPerChannel;

			if (useStaging)
			{

				stagingBuffer = new Buffer(isGraphicsDevice);
				stagingBuffer->usageflags = BufferUsage_TransferSource;
				stagingBuffer->SetMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				stagingBuffer->bufferSize = size;
				stagingBuffer->Setup();

				stagingBuffer->Map();
				memset(stagingBuffer->mapped, 0, size);
				stagingBuffer->Unmap();

				TF3D_SAFE_DELETE(stagingBuffer);
			}

			// We only need 1 as there will always be 1 mipmap in this application
			VkBufferImageCopy bufferCopyRegion;
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = sizeX;
			bufferCopyRegion.imageExtent.height = sizeY;
			bufferCopyRegion.imageExtent.depth = sizeZ;
			bufferCopyRegion.bufferOffset = 0;

			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.format = imageFormat;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = {sizeX, sizeY, sizeZ};
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			TF3D_VK_CALL(vkCreateImage(device, &imageCreateInfo, nullptr, &image));

			VkMemoryRequirements memoryRequirements;
			VkMemoryAllocateInfo memoryAllocateInfo = {};
			vkGetImageMemoryRequirements(device, image, &memoryRequirements);
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			if (isGraphicsDevice)
				memoryAllocateInfo.memoryTypeIndex = graphicsDevice->physicalDevice.GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			else
				memoryAllocateInfo.memoryTypeIndex = computeDevice->physicalDevice.GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			TF3D_VK_CALL(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &deviceMemory));


			
			isSetupOnGPU = true;
		}

		void GPUTexture::Destroy()
		{
			TF3D_ASSERT(isSetupOnGPU, "First call Setup");
			vkDestroyImageView(device, view, nullptr);
			vkDestroyImage(device, image, nullptr);
			if (sampler)
			{
				vkDestroySampler(device, sampler, nullptr);
			}
			vkFreeMemory(device, deviceMemory, nullptr);
			isSetupOnGPU = false;
		}

		void GPUTexture::GetHandle(void* h)
		{
			memcpy(h, &image, sizeof(image));
		}

		void GPUTexture::SetData(void* data)
		{
			TF3D_ASSERT(type == RendererAPI::GPUTextureType_2D, "GPUTexture::SetData(void* data) is only for 2D textures");
			TF3D_ASSERT(isSetupOnGPU, "First call Setup");
			TF3D_ASSERT(sizeX > 0 && sizeY > 0 && sizeZ > 0, "Texture sizes cannot be all 0");
			TF3D_ASSERT(data, "Data is null");
		}

		void GPUTexture::SetData(void* data, uint32_t depth)
		{
			TF3D_ASSERT(type == RendererAPI::GPUTextureType_3D || type == RendererAPI::GPUTextureType_Array, "GPUTexture::SetData(void* data, uint32_t depth) is only for 3D textures and texture arrays");
			TF3D_ASSERT(isSetupOnGPU, "Texture not setup on GPU (First call Setup)");
			TF3D_ASSERT(sizeX > 0 && sizeY > 0 && sizeZ > 0, "Texture sizes cannot be all 0");
			TF3D_ASSERT(data, "Data is null");
			TF3D_ASSERT(depth >= 0, "Depth must be greater than or equal to 0");
			if (depth > 256)
			{
				TF3D_LOG_WARN("Using 3D texture with depth {0} (might be unsupported on certain gpus)", depth);
			}
			UpdateInfo();
		}

		void GPUTexture::SetData(void* data, RendererAPI::GPUTextureCubemapFace face)
		{
			TF3D_ASSERT(type == RendererAPI::GPUTextureType_Cubemap, "GPUTexture::SetData(void* data, uint32_t depth) is only for 3D textures and texture arrays");
			TF3D_ASSERT(isSetupOnGPU, "Texture not setup on GPU (First call Setup)");
			TF3D_ASSERT(sizeX > 0 && sizeY > 0 && sizeZ > 0, "Texture sizes cannot be all 0");
			TF3D_ASSERT(data, "Data is null");
			UpdateInfo();
		}

		ImTextureID GPUTexture::GetImGuiID()
		{
			if(imGuiID)
				return imGuiID;
			imGuiID = (ImTextureID)(ImGui_ImplVulkan_AddTexture(sampler, view, imageLayout));
		}

		void GPUTexture::UseGraphicsDevice()
		{
			TF3D_ASSERT(!isSetupOnGPU, "Device can not be chaned after setup");
			isGraphicsDevice = true;
			graphicsDevice = GraphicsDevice::Get();
			device = graphicsDevice->handle;
		}

		void GPUTexture::UseComputeDevice()
		{
			TF3D_ASSERT(!isSetupOnGPU, "Device can not be chaned after setup");
			isGraphicsDevice = false;
			computeDevice = ComputeDevice::Get();
			device = computeDevice->handle;
		}

		void GPUTexture::UpdateInfo()
		{
			switch (type)
			{
			case TerraForge3D::RendererAPI::GPUTextureType_2D:
				sizeZ = 1;
				break;
			case TerraForge3D::RendererAPI::GPUTextureType_3D:
				break;
			case TerraForge3D::RendererAPI::GPUTextureType_Cubemap:
				sizeZ = 6;
				break;
			case TerraForge3D::RendererAPI::GPUTextureType_Array:
				break;
			default:
				TF3D_ASSERT(false, "Unknown GPUTextureType");
			}

			switch (format)
			{
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGB8I:
				bytesPerChannel = sizeof(unsigned char) * 3;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGB16I:
				bytesPerChannel = sizeof(unsigned short) * 3;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGB32F:
				bytesPerChannel = sizeof(float) * 3;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGBA8I:
				bytesPerChannel = sizeof(unsigned char) * 4;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGBA16I:
				bytesPerChannel = sizeof(unsigned short) * 4;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGBA32F:
				bytesPerChannel = sizeof(float) * 4;
				break;
			default:
				TF3D_ASSERT(false, "Unknown texture format");
			}
		}

	}

}

#endif