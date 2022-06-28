#include "Base/Vulkan/Pipeline.hpp"
#include "Base/Vulkan/Framebuffer.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"
#include "Base/Vulkan/Shader.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{
		Pipeline::Pipeline()
		{
		}

		Pipeline::~Pipeline()
		{
			if (autoDestory && isBuild)
			{
			}
		}

		void Pipeline::Setup()
		{
			
		}

		void Pipeline::Destory()
		{
			if (!isBuild)
				return;

			vkDestroyPipelineLayout(device->handle, pipelineLayout, nullptr);

			isBuild = false;
		}

		bool Pipeline::IsRebuildRequired(RendererAPI::FrameBuffer* f)
		{
			if (!currentFramebuffer.handle)
				return true;

			if (currentFramebuffer.handle != f)
				return true;


			if (currentFramebuffer.width != f->GetWidth() || currentFramebuffer.height != f->GetHeight())
				return true;

			return false;
		}

		bool Pipeline::Rebuild(RendererAPI::FrameBuffer* f, bool forceRebuild)
		{
			if (!IsRebuildRequired(f) && !forceRebuild)
				return true;

			currentFramebuffer.handle = reinterpret_cast<FrameBuffer*>(f);
			currentFramebuffer.width = f->GetWidth();
			currentFramebuffer.height = f->GetHeight();

			TF3D_ASSERT(shader->IsCompiled(), "Shader is not compiled");
			
			if (!device)
				device = GraphicsDevice::Get();

			VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
			vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
			vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
			vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
			vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
			inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyStateCreateInfo.topology = primitiveTopology;
			inputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnabled;

			VkViewport viewport{};
			viewport.x = viewportBegin[0];
			viewport.y = viewportBegin[1];
			viewport.width = currentFramebuffer.width;
			viewport.height = currentFramebuffer.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkExtent2D extent{};
			extent.height = currentFramebuffer.height;
			extent.width = currentFramebuffer.width;

			VkRect2D scissor{};
			scissor.offset = {0, 0};
			scissor.extent = extent;

			VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.viewportCount = 1;
			viewportStateCreateInfo.pViewports = &viewport;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.pScissors = &scissor;

			VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
			rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
			rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizationStateCreateInfo.polygonMode = polygonMode;
			rasterizationStateCreateInfo.lineWidth = 1.0f;
			rasterizationStateCreateInfo.cullMode = cullMode;
			rasterizationStateCreateInfo.frontFace = frontFace;
			rasterizationStateCreateInfo.depthBiasEnable = depthBiasEnable;
			rasterizationStateCreateInfo.depthBiasConstantFactor = depthBiasConstantFactor;
			rasterizationStateCreateInfo.depthBiasClamp = depthBiasClamp;
			rasterizationStateCreateInfo.depthBiasSlopeFactor = depthBiasSlopeFactor;

			VkPipelineMultisampleStateCreateInfo multisamplinStateCreateInfo{};
			multisamplinStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisamplinStateCreateInfo.sampleShadingEnable = VK_FALSE;
			multisamplinStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisamplinStateCreateInfo.minSampleShading = 1.0f; // Optional
			multisamplinStateCreateInfo.pSampleMask = nullptr; // Optional
			multisamplinStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optional
			multisamplinStateCreateInfo.alphaToOneEnable = VK_FALSE; // Optional

			VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
			colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachmentState.blendEnable = VK_FALSE;
			colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD; // Optional
			colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

			VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
			colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
			colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
			colorBlendStateCreateInfo.attachmentCount = 1;
			colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
			colorBlendStateCreateInfo.blendConstants[0] = 0.0f; // Optional
			colorBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
			colorBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
			colorBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional

			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
			dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
			dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

			VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo{};
			vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertexShaderStageCreateInfo.module = reinterpret_cast<Shader*>(shader)->vertexShaderModule;
			vertexShaderStageCreateInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{};
			fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentShaderStageCreateInfo.module = reinterpret_cast<Shader*>(shader)->fragmentShaderModule;
			fragmentShaderStageCreateInfo.pName = "main";
						
			VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

			VkPipelineLayoutCreateInfo layoutCreateInfo{};
			layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutCreateInfo.setLayoutCount = 0;
			layoutCreateInfo.pSetLayouts = nullptr;
			layoutCreateInfo.pushConstantRangeCount = 0;
			layoutCreateInfo.pPushConstantRanges = nullptr;

			TF3D_VK_CALL(vkCreatePipelineLayout(device->handle, &layoutCreateInfo, nullptr, &pipelineLayout));

			VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
			graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCreateInfo.stageCount = 2;
			graphicsPipelineCreateInfo.pStages = shaderStages;
			graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
			graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
			graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
			graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
			graphicsPipelineCreateInfo.pMultisampleState = &multisamplinStateCreateInfo;
			graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
			graphicsPipelineCreateInfo.layout = pipelineLayout;
			graphicsPipelineCreateInfo.renderPass = currentFramebuffer.handle->renderPass;
			graphicsPipelineCreateInfo.subpass = 0;
			// graphicsPipelineCreateInfo.basePipelineHandle = pipeline;
			graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

			TF3D_VK_CALL(vkCreateGraphicsPipelines(device->handle, VK_NULL_HANDLE /* pipelineCache */, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline));

			isBuild = true;
			return true;
		}

	}

}

#endif