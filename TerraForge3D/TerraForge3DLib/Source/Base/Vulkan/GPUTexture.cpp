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
			TF3D_ASSERT(type != RendererAPI::GPUTextureType_3D, "3D Textures are not yet supported on Vulkan Backend");

			UpdateInfo();

			
			uint64_t size = sizeX * sizeY * sizeZ * bytesPerChannel;


			stagingBuffer = new Buffer(isGraphicsDevice);
			stagingBuffer->usageflags = BufferUsage_TransferSource;
			stagingBuffer->SetMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			stagingBuffer->bufferSize = size;
			stagingBuffer->Setup();

			stagingBuffer->Map();
			memset(stagingBuffer->mapped, 0, size);
			stagingBuffer->Unmap();


			std::vector<VkBufferImageCopy> bufferCopyRegions;

			if (type == RendererAPI::GPUTextureType_2D)
			{
				// We only need 1 as there will always be 1 mipmap in this application
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = 0;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = sizeX;
				bufferCopyRegion.imageExtent.height = sizeY;
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = 0;

				bufferCopyRegions.push_back(bufferCopyRegion);
			}
			else if (type == RendererAPI::GPUTextureType_2D)
			{
				VkDeviceSize offset = 0;
				for (uint32_t layer = 0; layer < sizeZ; layer++)
				{
					VkBufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferCopyRegion.imageSubresource.mipLevel = 0;
					bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent.width = sizeX;
					bufferCopyRegion.imageExtent.height = sizeY;
					bufferCopyRegion.imageExtent.depth = 1;
					bufferCopyRegion.bufferOffset = offset;
					offset += sizeX * sizeY * 1 * bytesPerChannel;

					bufferCopyRegions.push_back(bufferCopyRegion);
				}
			}

			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = imageType;
			imageCreateInfo.format = imageFormat;
			imageCreateInfo.mipLevels = 1;				
			imageCreateInfo.arrayLayers = (type == RendererAPI::GPUTextureType_Array ? sizeZ : 1);
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = {sizeX, sizeY, 1};
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

			TF3D_VK_CALL(vkCreateImage(device, &imageCreateInfo, nullptr, &image));

			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(device, image, &memoryRequirements);
			
			VkMemoryAllocateInfo memoryAllocateInfo = {};
			memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = logicalDevice->physicalDevice.GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			TF3D_VK_CALL(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &deviceMemory));
			TF3D_VK_CALL(vkBindImageMemory(device, image, deviceMemory, 0));

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = type == RendererAPI::GPUTextureType_Array ? sizeZ : 1;

			// Image barier for optimal image (target)
			// Optimal image will be used as a destination for the copy
			VkCommandBuffer commandBuffer = logicalDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			SetImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
			vkCmdCopyBufferToImage(commandBuffer, stagingBuffer->handle, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());
			SetImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout, subresourceRange);
			logicalDevice->FlushCommandBuffer(commandBuffer);
			

			TF3D_SAFE_DELETE(stagingBuffer);

			// Create the sampler

			VkSamplerCreateInfo samplerCreateInfo = {};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			if (type == RendererAPI::GPUTextureType_Cubemap)
			{
				samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}
			else
			{
				samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;				
			}

			samplerCreateInfo.mipLodBias = 0.0f;
			samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerCreateInfo.minLod = 0.0f;
			samplerCreateInfo.maxLod = 1.0f; // only 1 mip level


			samplerCreateInfo.maxAnisotropy = 1.0f;
			samplerCreateInfo.anisotropyEnable = VK_FALSE;
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			TF3D_VK_CALL(vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler));

			// Create Image View
			// Textures are not directly accesed by shaders and are
			// abstracted by image vies containaing additional information
			// and subresource ranges
			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.viewType = imageViewType;
			viewCreateInfo.format = imageFormat;
			if(isRGBA)
				viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			else
				viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B };
			//viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			//viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange = subresourceRange;
			viewCreateInfo.image = image;
			TF3D_VK_CALL(vkCreateImageView(device, &viewCreateInfo, nullptr, &view));


			isSetupOnGPU = true;
		}

		void GPUTexture::Destroy()
		{
			TF3D_ASSERT(isSetupOnGPU, "First call Setup");
			vkDeviceWaitIdle(device);
			vkDestroyImageView(device, view, nullptr);
			vkDestroyImage(device, image, nullptr);
			if (sampler)
			{
				vkDestroySampler(device, sampler, nullptr);
			}
			vkFreeMemory(device, deviceMemory, nullptr);
			imGuiID = VK_NULL_HANDLE;
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
			UpdateInfo();

			uint64_t size = sizeX * sizeY * sizeZ * bytesPerChannel;


			stagingBuffer = new Buffer(isGraphicsDevice);
			stagingBuffer->usageflags = BufferUsage_TransferSource;
			stagingBuffer->SetMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			stagingBuffer->bufferSize = size;
			stagingBuffer->Setup();

			stagingBuffer->Map();
			stagingBuffer->SetData(data, size);
			stagingBuffer->Unmap();



			// We only need 1 as there will always be 1 mipmap in this application
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = sizeX;
			bufferCopyRegion.imageExtent.height = sizeY;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = 0;

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;

			// Image barier for optimal image (target)
			// Optimal image will be used as a destination for the copy
			VkCommandBuffer commandBuffer = logicalDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			SetImageLayout(commandBuffer, image, imageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
			vkCmdCopyBufferToImage(commandBuffer, stagingBuffer->handle, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
			SetImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout, subresourceRange);
			logicalDevice->FlushCommandBuffer(commandBuffer);

			TF3D_SAFE_DELETE(stagingBuffer);

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

			uint64_t size = sizeX * sizeY * 1 * bytesPerChannel;


			stagingBuffer = new Buffer(isGraphicsDevice);
			stagingBuffer->usageflags = BufferUsage_TransferSource;
			stagingBuffer->SetMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			stagingBuffer->bufferSize = size;
			stagingBuffer->Setup();

			stagingBuffer->Map();
			stagingBuffer->SetData(data, size);
			stagingBuffer->Unmap();



			// We only need 1 as there will always be 1 mipmap in this application
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = depth;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = sizeX;
			bufferCopyRegion.imageExtent.height = sizeY;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = size * depth;

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;
			subresourceRange.baseArrayLayer = depth;

			// Image barier for optimal image (target)
			// Optimal image will be used as a destination for the copy
			VkCommandBuffer commandBuffer = logicalDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			SetImageLayout(commandBuffer, image, imageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
			vkCmdCopyBufferToImage(commandBuffer, stagingBuffer->handle, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
			SetImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout, subresourceRange);
			logicalDevice->FlushCommandBuffer(commandBuffer);

			TF3D_SAFE_DELETE(stagingBuffer);
		}

		void GPUTexture::SetData(void* data, RendererAPI::GPUTextureCubemapFace face)
		{
			TF3D_ASSERT(type == RendererAPI::GPUTextureType_Cubemap, "GPUTexture::SetData(void* data, uint32_t depth) is only for 3D textures and texture arrays");
			TF3D_ASSERT(isSetupOnGPU, "Texture not setup on GPU (First call Setup)");
			TF3D_ASSERT(sizeX > 0 && sizeY > 0 && sizeZ > 0, "Texture sizes cannot be all 0");
			TF3D_ASSERT(data, "Data is null");
			UpdateInfo();
		}

		void GPUTexture::GetData(void* data)
		{
			TF3D_ASSERT(isSetupOnGPU, "Texture not setup on GPU (First call Setup)");

			uint64_t size = sizeX * sizeY * sizeZ * bytesPerChannel;


			stagingBuffer = new Buffer(isGraphicsDevice);
			stagingBuffer->usageflags = BufferUsage_TransferDestination;
			stagingBuffer->SetMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			stagingBuffer->bufferSize = size;
			stagingBuffer->Setup();




			// We only need 1 as there will always be 1 mipmap in this application
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			if (type == RendererAPI::GPUTextureType_Array)
				bufferCopyRegion.imageSubresource.layerCount = sizeZ;
			else
				bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = sizeX;
			bufferCopyRegion.imageExtent.height = sizeY;
			if (type == RendererAPI::GPUTextureType_3D)
				bufferCopyRegion.imageExtent.depth = sizeZ;
			else
				bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = 0;

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			if (type == RendererAPI::GPUTextureType_Array)
				subresourceRange.layerCount = sizeZ;
			else
				subresourceRange.layerCount = 1;

			// Image barier for optimal image (target)
			// Optimal image will be used as a destination for the copy
			VkCommandBuffer commandBuffer = logicalDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			SetImageLayout(commandBuffer, image, imageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
			vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer->handle, 1, &bufferCopyRegion);
			SetImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imageLayout, subresourceRange);
			logicalDevice->FlushCommandBuffer(commandBuffer);

			stagingBuffer->Map();
			memcpy(data, stagingBuffer->mapped, stagingBuffer->bufferSize);
			stagingBuffer->Unmap();

			TF3D_SAFE_DELETE(stagingBuffer);
		}


		ImTextureID GPUTexture::GetImGuiID()
		{
			if(imGuiID)
				return imGuiID;
			imGuiID = (ImTextureID)(ImGui_ImplVulkan_AddTexture(sampler, view, imageLayout));
			return imGuiID;
		}

		void GPUTexture::UseGraphicsDevice()
		{
			TF3D_ASSERT(!isSetupOnGPU, "Device can not be chaned after setup");
			isGraphicsDevice = true;
			logicalDevice = GraphicsDevice::Get();
			device = logicalDevice->handle;
		}

		void GPUTexture::UseComputeDevice()
		{
			TF3D_ASSERT(!isSetupOnGPU, "Device can not be chaned after setup");
			isGraphicsDevice = false;
			logicalDevice = ComputeDevice::Get();
			device = logicalDevice->handle;
		}

		void GPUTexture::UpdateInfo()
		{
			switch (type)
			{
			case TerraForge3D::RendererAPI::GPUTextureType_2D:
				sizeZ = 1;
				imageType = VK_IMAGE_TYPE_2D;
				break;
			case TerraForge3D::RendererAPI::GPUTextureType_3D:
				imageType = VK_IMAGE_TYPE_3D;
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
				imageFormat = VK_FORMAT_R8G8B8_UNORM;
				isRGBA = false;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGB16I:
				bytesPerChannel = sizeof(unsigned short) * 3;
				imageFormat = VK_FORMAT_R16G16B16_UNORM;
				isRGBA = false;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGB32F:
				bytesPerChannel = sizeof(float) * 3;
				imageFormat = VK_FORMAT_R32G32B32_SFLOAT;
				isRGBA = false;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGBA8I:
				bytesPerChannel = sizeof(unsigned char) * 4;
				imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
				isRGBA = true;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGBA16I:
				bytesPerChannel = sizeof(unsigned short) * 4;
				imageFormat = VK_FORMAT_R16G16B16A16_UNORM;
				isRGBA = true;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGBA32F:
				bytesPerChannel = sizeof(float) * 4;
				imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
				isRGBA = true;
				break;
			default:
				TF3D_ASSERT(false, "Unknown texture format");
			}
		}

		void GPUTexture::SetImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
		{
			// Create an image memory barrier object
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// be fore it will be transitioned to the new layout
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				commandBuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}
				
	}

}

#endif