#pragma once
#include "Base/Vulkan/Core.hpp"
#include "Base/Renderer/GPUTexture.hpp"
#include "Base/Vulkan/Buffer.hpp"

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
			virtual ImTextureID GetImGuiID() override;


			virtual void UseGraphicsDevice();
			virtual void UseComputeDevice();

			void UpdateInfo();

		public:
			VkImage image = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VkSampler sampler = VK_NULL_HANDLE;
			VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
			VkImageLayout imageLayout;
			VkDevice device = VK_NULL_HANDLE;

			ComputeDevice* computeDevice = nullptr;
			GraphicsDevice * graphicsDevice = nullptr;
			bool isGraphicsDevice = true;

			Buffer* stagingBuffer = nullptr;

			VkBool32 useStaging = VK_TRUE;

			VkFormat imageFormat = VK_FORMAT_R8G8B8_UINT;
			ImTextureID imGuiID = nullptr;
			uint32_t bytesPerChannel = 1;
		};

	}

}


#endif