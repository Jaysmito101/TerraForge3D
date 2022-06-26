#pragma once
#include "Base/Renderer/Framebuffer.hpp"
#include "Base/Vulkan/Core.hpp"
#include "Base/Vulkan/LogicalDevice.hpp"
#include "Base/Vulkan/GPUTexture.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{

		class FrameBuffer : public RendererAPI::FrameBuffer
		{
		public:
			FrameBuffer();
			~FrameBuffer();

			virtual void Setup() override;
			virtual void Destroy() override;
			virtual void Clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) override;
			virtual void* GetNativeHandle(void* handle = nullptr) override;
			virtual ImTextureID GetColorAttachmentImGuiID(int index = 0);
			virtual void* GetColorAttachmentHandle(int index, void* handle = nullptr) override;
			virtual void* GetDepthAttachmentHandle(void* handle = nullptr) override;
			virtual void* ReadPixel(uint32_t x, uint32_t y, int index = 0, void* data = nullptr) override;

			void SetupRenderPass();
			void SetupAttachments();

			inline FrameBuffer* SetDevice(LogicalDevice* d) { TF3D_ASSERT(!setupOnGPU, "Cannot set device after framebuffer has been setup [Call Destroy first]"); this->device = d; }

		public:
			VkFramebuffer handle = VK_NULL_HANDLE;
			VkRenderPass renderPass = VK_NULL_HANDLE;
		
			std::vector<VkImageView> attachments;

			GPUTexture* colorAttachments[4] = { nullptr, nullptr, nullptr, nullptr };
			GPUTexture* depthAttachment = nullptr;

			LogicalDevice* device = nullptr;
		};

	}

}

#endif