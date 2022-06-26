#include "Base/Vulkan/Framebuffer.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{
		
		FrameBuffer::FrameBuffer()
		{
		}

		FrameBuffer::~FrameBuffer()
		{
			if (autoDestroy && setupOnGPU)
			{
				this->Destroy();
			}
		}

		void FrameBuffer::SetupRenderPass()
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = VK_FORMAT_A2B10G10R10_SINT_PACK32; // TODO
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colorAttachmentRef{};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = 1;
			renderPassCreateInfo.pAttachments = &colorAttachment;
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;

			TF3D_VK_CALL(vkCreateRenderPass(device->handle, &renderPassCreateInfo, nullptr, &renderPass));


		}

		void FrameBuffer::SetupAttachments()
		{
			attachments.clear();

			for (int i = 0; i < colorAttachmentCount; i++)
			{
				colorAttachments[i] = new GPUTexture();
				colorAttachments[i]->SetSize(width, height, 1);
				colorAttachments[i]->SetType(RendererAPI::GPUTextureType_2D);
				switch (colorAttachmentFromats[i])
				{
				case RendererAPI::FrameBufferAttachmentFormat_RGBA8:
					colorAttachments[i]->SetFormat(RendererAPI::GPUTextureFormat_RGBA8I);
					break;
				case RendererAPI::FrameBufferAttachmentFormat_RGBA16:
					colorAttachments[i]->SetFormat(RendererAPI::GPUTextureFormat_RGBA16I);
					break;
				case RendererAPI::FrameBufferAttachmentFormat_RGBA32:
					colorAttachments[i]->SetFormat(RendererAPI::GPUTextureFormat_RGBA32F);
					break;
				case RendererAPI::FrameBufferAttachmentFormat_R32I:
					colorAttachments[i]->SetFormat(RendererAPI::GPUTextureFormat_R32I);
					break;

				default:
					TF3D_ASSERT(false, "Unknown FrameBuffer attachment format");
				}
				colorAttachments[i]->UseGraphicsDevice();
				colorAttachments[i]->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // TODO: Look this up
				colorAttachments[i]->Setup();
			
				attachments.push_back(colorAttachments[i]->view);
			}

			// TODO: Implement depth textures
			// attachments.push_back(depthAttachment->view);
		}

		void FrameBuffer::Setup()
		{
			TF3D_ASSERT(setupOnGPU == false, "FrameBuffer has already been setup call Destory first");
			TF3D_ASSERT(colorAttachmentCount >= 1, "Color attachment count cannot be less than 1");

			if (!this->device)
				this->device = GraphicsDevice::Get();

			// Setup Render Pass
			SetupRenderPass();
			
			// Setup Attachments
			SetupAttachments();

			// Create the framebuffer
			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = renderPass;
			framebufferCreateInfo.attachmentCount = attachments.size();
			framebufferCreateInfo.pAttachments = attachments.data();
			framebufferCreateInfo.width = width;
			framebufferCreateInfo.height = height;
			framebufferCreateInfo.layers = 1;

			TF3D_VK_CALL(vkCreateFramebuffer(device->handle, &framebufferCreateInfo, nullptr, &handle))

			setupOnGPU = true;
		}

		void FrameBuffer::Destroy()
		{
			TF3D_ASSERT(setupOnGPU, "FrameBuffer has not yet been setup call Setup first");
			vkDestroyFramebuffer(this->device->handle, handle, nullptr);
			vkDestroyRenderPass(this->device->handle, renderPass, nullptr);
			for (int i = 0; i < colorAttachmentCount; i++)
			{
				colorAttachments[i]->Destroy();
				TF3D_SAFE_DELETE(colorAttachments[i]);
			}
			if (hasDepthAttachment)
			{
				depthAttachment->Destroy();
				TF3D_SAFE_DELETE(depthAttachment);
			}
			setupOnGPU = false;
		}

		void FrameBuffer::Clear(float r, float g, float b, float a)
		{
		}

		void* FrameBuffer::GetNativeHandle(void* h)
		{
		}

		ImTextureID GetColorAttachmenntImGuiID(int index)
		{
		}

		void* FrameBuffer::GetColorAttachmentHandle(int index, void* h)
		{
		}

		void* FrameBuffer::GetDepthAttachmentHandle(void* h)
		{
		}

		void* ReadPixel(uint32_t x, uint32_t y, int index, void* data)
		{
		}
	}

}

#endif