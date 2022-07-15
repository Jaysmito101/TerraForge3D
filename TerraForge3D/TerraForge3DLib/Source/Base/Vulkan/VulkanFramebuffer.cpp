#include "Base/Vulkan/Framebuffer.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"

#include "imgui/backends/imgui_impl_vulkan.h"


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
			std::vector<VkAttachmentDescription> attachmentDescriptions;
			std::vector<VkAttachmentReference> attachmentReferences;

			for (int i = 0; i < colorAttachmentCount; i++)
			{
				VkAttachmentDescription colorAttachment{};
				
				colorAttachment.format = VK_FORMAT_A2B10G10R10_SINT_PACK32; // TODO
				switch (colorAttachmentFromats[i])
				{
				case RendererAPI::FrameBufferAttachmentFormat_RGBA8:
					colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
					break;
				case RendererAPI::FrameBufferAttachmentFormat_RGBA16:
					colorAttachment.format = VK_FORMAT_R16G16B16A16_UNORM;
					break;
				case RendererAPI::FrameBufferAttachmentFormat_RGBA32:
					colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
					break;
				case RendererAPI::FrameBufferAttachmentFormat_R32I:
					colorAttachment.format = VK_FORMAT_R32_SINT;
					break;

				default:
					TF3D_ASSERT(false, "Unknown FrameBuffer attachment format");
				}

				colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				// colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
				attachmentDescriptions.push_back(colorAttachment);

				VkAttachmentReference colorAttachmentRef{};
				colorAttachmentRef.attachment = i;
				// colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorAttachmentRef.layout = VK_IMAGE_LAYOUT_GENERAL;
				attachmentReferences.push_back(colorAttachmentRef);
			}

			VkAttachmentDescription depthAttachmentDescription{};
			depthAttachmentDescription.format = VK_FORMAT_D24_UNORM_S8_UINT;
			depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			if(hasDepthAttachment)
				attachmentDescriptions.push_back(depthAttachmentDescription);

			VkAttachmentReference depthAttachmentReference{};
			depthAttachmentReference.attachment = colorAttachmentCount;
			depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = colorAttachmentCount;
			subpass.pColorAttachments = attachmentReferences.data();
			if (hasDepthAttachment)
				subpass.pDepthStencilAttachment = &depthAttachmentReference;
			else
				subpass.pDepthStencilAttachment = nullptr;

			VkSubpassDependency dependency{};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;			

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;
			renderPassCreateInfo.dependencyCount = 1;
			renderPassCreateInfo.pDependencies = &dependency;

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
				// colorAttachments[i]->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // TODO: Look this up
				colorAttachments[i]->imageLayout = VK_IMAGE_LAYOUT_GENERAL; // TODO: Look this up
				colorAttachments[i]->usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				colorAttachments[i]->Setup();
			
				attachments.push_back(colorAttachments[i]->view);
			}

			// TODO: Implement depth textures
			// attachments.push_back(depthAttachment->view);
		}

		void FrameBuffer::SetupDepthAttachment()
		{
			// Create the image

			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.extent.width = width;
			imageCreateInfo.extent.height = height;
			imageCreateInfo.extent.depth = 1;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
			// imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			TF3D_VK_CALL(vkCreateImage(device->handle, &imageCreateInfo, nullptr, &depthAttachment.handle));

			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(device->handle, depthAttachment.handle, &memoryRequirements);
			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memoryRequirements.size;
			allocInfo.memoryTypeIndex = device->physicalDevice.GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			
			TF3D_VK_CALL(vkAllocateMemory(device->handle, &allocInfo, nullptr, &depthAttachment.deviceMemory));

			TF3D_VK_CALL(vkBindImageMemory(device->handle, depthAttachment.handle, depthAttachment.deviceMemory, 0));

			// Create the image view
			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = depthAttachment.handle;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.subresourceRange.levelCount = 1;

			TF3D_VK_CALL(vkCreateImageView(device->handle, &imageViewCreateInfo, nullptr, &depthAttachment.view));

			VkCommandBuffer commandBuffer = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			GPUTexture::SetImageLayout(commandBuffer, depthAttachment.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, imageViewCreateInfo.subresourceRange);
			device->FlushCommandBuffer(commandBuffer);

			attachments.push_back(depthAttachment.view);
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

			// Setup Depth Attachment
			if(hasDepthAttachment)
				SetupDepthAttachment();

			// Create the framebuffer
			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = renderPass;
			framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
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
			colorAttachemtImGuiIDs[0] = nullptr;
			colorAttachemtImGuiIDs[1] = nullptr;
			colorAttachemtImGuiIDs[2] = nullptr;
			colorAttachemtImGuiIDs[3] = nullptr;
			if (hasDepthAttachment)
			{
				vkDeviceWaitIdle(device->handle);
				vkDestroyImageView(device->handle, depthAttachment.view, nullptr);
				vkDestroyImage(device->handle, depthAttachment.handle, nullptr);
				vkFreeMemory(device->handle, depthAttachment.deviceMemory, nullptr);
			}
			setupOnGPU = false;
		}

		void FrameBuffer::Clear(float r, float g, float b, float a)
		{
			TF3D_ASSERT(false, "Not yet implemented");
		}

		void* FrameBuffer::GetNativeHandle(void* h)
		{
			TF3D_ASSERT(setupOnGPU, "Cannot get native handle before framebuffer is setup");
			if (h)
				memcpy(h, &handle, sizeof(handle));
			return handle;
		}

		ImTextureID FrameBuffer::GetColorAttachmentImGuiID(int index)
		{
			TF3D_ASSERT(index < 4 && index >= 0, "Color attachment index must be greater than equal to 0 and less than 4");
			TF3D_ASSERT(index < colorAttachmentCount, "Color attachment index out of bounds");
			TF3D_ASSERT(setupOnGPU, "Cannot get ImGui ID before framebuffer is setup");
			if (colorAttachemtImGuiIDs[index])
				return colorAttachemtImGuiIDs[index];
			GPUTexture* texture = colorAttachments[index];
			colorAttachemtImGuiIDs[index] = ImGui_ImplVulkan_AddTexture(texture->sampler, texture->view, texture->imageLayout);
			return colorAttachemtImGuiIDs[index];
		}

		void* FrameBuffer::GetColorAttachmentHandle(int index, void* h)
		{
			TF3D_ASSERT(index < 4 && index >= 0, "Color attachment index must be greater than equal to 0 and less than 4");
			TF3D_ASSERT(index < colorAttachmentCount, "Color attachment index out of bounds");
			TF3D_ASSERT(setupOnGPU, "Cannot get color atachment handle before framebuffer is setup");
			if (h)
				memcpy(h, &colorAttachments[index], sizeof(colorAttachments[index]));
			return colorAttachments[index];
		}

		void* FrameBuffer::GetDepthAttachmentHandle(void* h)
		{
			TF3D_ASSERT(setupOnGPU, "Cannot get color atachment handle before framebuffer is setup");
			if (h)
				memcpy(h, &depthAttachment.handle, sizeof(depthAttachment.handle));
			return depthAttachment.handle;
		}

		void* FrameBuffer::ReadPixel(uint32_t x, uint32_t y, int index, void* data)
		{
			TF3D_ASSERT(index < 4 && index >= 0, "Color attachment index must be greater than equal to 0 and less than 4");
			TF3D_ASSERT(index < colorAttachmentCount, "Color attachment index out of bounds");
			TF3D_ASSERT(setupOnGPU, "Cannot read pixel handle before framebuffer is setup");
			GPUTexture* texture = colorAttachments[index];
			return texture->ReadPixel(x, y, data);
		}
	}

}

#endif