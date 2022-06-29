#pragma once
#include "Base/Vulkan/Core.hpp"
#include "Base/Vulkan/LogicalDevice.hpp"
#include "Base/Renderer/Pipeline.hpp"

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{
		class FrameBuffer;

		class Pipeline : public RendererAPI::Pipeline
		{
		public:
			Pipeline();
			~Pipeline();

			virtual void Setup() override;
			virtual void Destory() override;
			virtual bool Rebuild(RendererAPI::FrameBuffer* framebuffer, bool forceRebuild = false) override;

			bool IsRebuildRequired(RendererAPI::FrameBuffer* framebuffer);

		public:
			struct
			{
				FrameBuffer* handle = nullptr;
				uint32_t width = 0;
				uint32_t height = 0;
			} currentFramebuffer;

			LogicalDevice* device = nullptr;

			// Pipeline Options
			
			// Vertex Input Assembly
			VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			VkBool32 primitiveRestartEnabled = VK_FALSE;

			// Rasterizer
			VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
			VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
			VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
			VkBool32 depthBiasEnable = VK_FALSE;
			float depthBiasConstantFactor = 0.0f;
			float depthBiasClamp = 0.0f;
			float depthBiasSlopeFactor = 0.0f;

			// Dynamic State
			std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT};

			// Vulkan Pipeline Data
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkPipelineCache pipelineCache = VK_NULL_HANDLE;
			VkPipeline pipeline = VK_NULL_HANDLE;


			bool isBuild = false;

		};

	}

}

#endif