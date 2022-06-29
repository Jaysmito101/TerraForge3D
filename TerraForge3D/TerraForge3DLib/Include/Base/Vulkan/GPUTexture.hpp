#pragma once
#include "Base/Vulkan/Core.hpp"
#include "Base/Renderer/GPUTexture.hpp"
#include "Base/Vulkan/Buffer.hpp"
#include "Base/Vulkan/LogicalDevice.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{

		class GraphicsDevice;
		class ComputeDevice;

		class GPUTexture : public RendererAPI::GPUTexture
		{
		public:
			GPUTexture();
			~GPUTexture();

			virtual void Setup() override;
			virtual void Destroy() override;
			virtual void SetData(void* data) override;
			virtual void SetData(void* data, uint32_t depth) override;
			virtual void SetData(void* data, RendererAPI::GPUTextureCubemapFace face) override;
			virtual void GetHandle(void* handle) override;
			virtual void GetData(void* data) override;
			virtual ImTextureID GetImGuiID() override;


			virtual void UseGraphicsDevice();
			virtual void UseComputeDevice();

			void UpdateInfo();
		// private:
			static void SetImageLayout(
				VkCommandBuffer cmdBuffer,
				VkImage image,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkImageSubresourceRange subresourceRange,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		public:
			VkImage image = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VkSampler sampler = VK_NULL_HANDLE;
			VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			VkDevice device = VK_NULL_HANDLE;
			LogicalDevice* logicalDevice = nullptr;
			
			bool isGraphicsDevice = true;

			Buffer* stagingBuffer = nullptr;

			bool isRGBA = false;

			VkImageUsageFlags usage = 0;
			VkFormat imageFormat = VK_FORMAT_R8G8B8_UINT;
			VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
			VkImageType imageType = VK_IMAGE_TYPE_2D;
			ImTextureID imGuiID = nullptr;
			uint32_t bytesPerChannel = 1;
		};

	}

}


#endif