#include "Base/Vulkan/Pipeline.hpp"
#include "Base/Vulkan/Framebuffer.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"
#include "Base/Vulkan/Shader.hpp"
#include "Base/DS/Mesh.hpp"

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
				Destory();
			}
		}

		void Pipeline::Setup()
		{
			isSetup = true;
		}

		void Pipeline::Destory()
		{
			if (!isBuild)
				return;

			vkDestroyPipelineLayout(device->handle, pipelineLayout, nullptr);

			isBuild = false;
			isSetup = false;
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

			VkVertexInputBindingDescription vertexInputBindingDescription{};
			vertexInputBindingDescription.binding = 0;
			vertexInputBindingDescription.stride = sizeof(Vertex);
			vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputAttributeDescription vertexInputAttributeDescription[4];
			vertexInputAttributeDescription[0].binding = 0;
			vertexInputAttributeDescription[0].location = 0;
			vertexInputAttributeDescription[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			vertexInputAttributeDescription[0].offset = offsetof(Vertex, position);

			vertexInputAttributeDescription[1].binding = 0;
			vertexInputAttributeDescription[1].location = 1;
			vertexInputAttributeDescription[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			vertexInputAttributeDescription[1].offset = offsetof(Vertex, texCoord);

			vertexInputAttributeDescription[2].binding = 0;
			vertexInputAttributeDescription[2].location = 2;
			vertexInputAttributeDescription[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			vertexInputAttributeDescription[2].offset = offsetof(Vertex, normal);

			vertexInputAttributeDescription[3].binding = 0;
			vertexInputAttributeDescription[3].location = 3;
			vertexInputAttributeDescription[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			vertexInputAttributeDescription[3].offset = offsetof(Vertex, extra);

			VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
			vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
			vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
			vertexInputStateCreateInfo.vertexAttributeDescriptionCount = TF3D_STATIC_ARRAY_SIZE(vertexInputAttributeDescription);
			vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescription;

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
			inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyStateCreateInfo.topology = primitiveTopology;
			inputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnabled;

			viewport.x = static_cast<float>(viewportBegin[0]);
			viewport.y = static_cast<float>(viewportBegin[1]);
			viewport.width = static_cast<float>(currentFramebuffer.width);
			viewport.height = static_cast<float>(currentFramebuffer.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			scissor.offset = {0, 0};
			scissor.extent.height = currentFramebuffer.height;
			scissor.extent.width = currentFramebuffer.width;

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

			std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
			for (int i = 0; i < currentFramebuffer.handle->GetColorAttachmentCount(); i++)
			{
				VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
				colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				colorBlendAttachmentState.blendEnable = VK_FALSE;
				colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
				colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
				colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD; // Optional
				colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
				colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
				colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
				colorBlendAttachmentStates.push_back(colorBlendAttachmentState);
			}

			VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
			colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
			colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
			colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
			colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();
			colorBlendStateCreateInfo.blendConstants[0] = 0.0f; // Optional
			colorBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
			colorBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
			colorBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional

			VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
			depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
			depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
			depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
			depthStencilStateCreateInfo.minDepthBounds = 0.0f;
			depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
			depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
			depthStencilStateCreateInfo.front = {};
			depthStencilStateCreateInfo.back = {};

			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
			dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
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

			VkPushConstantRange pushConstantsRange{};
			pushConstantsRange.offset = 0;
			pushConstantsRange.size = reinterpret_cast<Shader*>(shader)->pushConstantsSize;
			pushConstantsRange.stageFlags =VK_SHADER_STAGE_ALL_GRAPHICS;

			VkPipelineLayoutCreateInfo layoutCreateInfo{};
			layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutCreateInfo.setLayoutCount = 0;
			layoutCreateInfo.pSetLayouts = nullptr;
			layoutCreateInfo.pushConstantRangeCount = 1;
			layoutCreateInfo.pPushConstantRanges = &pushConstantsRange;

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
			graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
			graphicsPipelineCreateInfo.layout = pipelineLayout;
			graphicsPipelineCreateInfo.renderPass = currentFramebuffer.handle->renderPass;
			graphicsPipelineCreateInfo.subpass = 0;
			// graphicsPipelineCreateInfo.basePipelineHandle = pipeline;
			graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			if (currentFramebuffer.handle->HasDepthAttachment())
				graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;

			TF3D_VK_CALL(vkCreateGraphicsPipelines(device->handle, VK_NULL_HANDLE /* pipelineCache */, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline));

			isBuild = true;
			return true;
		}

	}

}

#endif