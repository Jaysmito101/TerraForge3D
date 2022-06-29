#include "Base/Vulkan/NativeRender.hpp"
#include "Base/Vulkan/Context.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"
#include "Base/Vulkan/FrameBuffer.hpp"
#include "Base/Vulkan/Pipeline.hpp"
#include "Base/Vulkan/NativeMesh.hpp"
#include "Base/Vulkan/Shader.hpp"
#include "Base/Renderer/Camera.hpp"

#include "Base/DS/Mesh.hpp"

#include <stack>

#ifdef TF3D_VULKAN_BACKEND

namespace TerraForge3D
{

	namespace Vulkan
	{

		NativeRenderer::NativeRenderer()
		{
			this->context = reinterpret_cast<Context*>(Context::Get());
			this->device = GraphicsDevice::Get();
		}

		NativeRenderer::~NativeRenderer()
		{
		}

		void NativeRenderer::Flush()
		{
			Vulkan::FrameBuffer* framebuffer = nullptr;
			Vulkan::Pipeline* pipeline = nullptr;
			Vulkan::NativeMesh* lastRenderedMesh = nullptr;
			Vulkan::Shader* shader = nullptr;
			RendererAPI::Camera* camera = nullptr;

			GraphicsDevice* device = GraphicsDevice::Get();
			VkCommandBuffer commandBuffer = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
				
			std::vector<VkRenderPassBeginInfo> renderPassBeginInfos;
			std::vector<std::pair<VkBuffer, VkDeviceSize>> buffers;

			std::stack<void*> rendererStack;

			while (!rendererQueue.empty())
			{
				auto [command, params] = rendererQueue.front();
				void* param = params.custom;
				rendererQueue.pop();
				switch (command)
				{
				case RendererCommand_Clear:
				{
					TF3D_ASSERT(param, "Parameter is null");
					TF3D_ASSERT(framebuffer, "Cannot clear without any bound framebuffer");
					float* clearColor = reinterpret_cast<float*>(param);
					// TODO: Implement
					break;
				}
				case RendererCommand_BindFrameBuffer:
				{
					if (framebuffer)
						vkCmdEndRenderPass(commandBuffer);
					TF3D_ASSERT(param, "Parameter is null");
					framebuffer = reinterpret_cast<Vulkan::FrameBuffer*>(param);
					TF3D_ASSERT(framebuffer->IsSetup(), "Framebuffer not yet setup");
					VkRenderPassBeginInfo renderPassBeginInfo{};
					renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassBeginInfo.renderPass = framebuffer->renderPass;
					renderPassBeginInfo.framebuffer = framebuffer->handle;
					renderPassBeginInfo.renderArea.offset = {0, 0};
					renderPassBeginInfo.renderArea.extent = { framebuffer->GetHeight(), framebuffer->GetWidth() };
					// TODO: Fixme for multiple attachments
					VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
					clearColor.depthStencil.depth = 0.0;
					clearColor.depthStencil.stencil = 0.0;
					renderPassBeginInfo.clearValueCount = 1;
					renderPassBeginInfo.pClearValues = &clearColor;
					renderPassBeginInfos.push_back(renderPassBeginInfo);
					vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfos.back(), VK_SUBPASS_CONTENTS_INLINE);
					break;
				}
				case RendererCommand_BindPipeline:
				{
					TF3D_ASSERT(param, "Parameter is null");
					TF3D_ASSERT(framebuffer, "Binding renderer pipeline without a framebuffer bound");
					pipeline = reinterpret_cast<Vulkan::Pipeline*>(param);
					TF3D_ASSERT(pipeline->IsSetup(), "Pipeline is not yet setup");
					shader = reinterpret_cast<Vulkan::Shader*>(pipeline->shader);
					// mousePickIDUniformLocation = glGetUniformLocation(shader->handle, "_MousePickID"); // TEMP here
					TF3D_ASSERT(shader, "Shader is null");
					TF3D_ASSERT(shader->IsCompiled(), "Shader is not compiled");
					pipeline->Rebuild(framebuffer);
					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
					break;
				}
				/*
				case RendererCommand_BindCamera:
				{
					TF3D_ASSERT(param, "Parameter is null");
					TF3D_ASSERT(pipeline, "Binding camera without a pipeline bound");
					camera = reinterpret_cast<RendererAPI::Camera*>(param);
					camera->RecalculateMatrices();
					glUniformMatrix4fv(pvMatLocation, 1, GL_FALSE, glm::value_ptr(camera->matrices.pv));
					break;
				}
				*/
				case RendererCommand_Draw:
				{
					TF3D_ASSERT(pipeline, "Cannot draw meshes without a pipeline bound");
					TF3D_ASSERT(param, "Parameter is null");
					Mesh* mesh = reinterpret_cast<Mesh*>(param);
					lastRenderedMesh = reinterpret_cast<Vulkan::NativeMesh*>(mesh->GetNativeMesh());
					TF3D_ASSERT(lastRenderedMesh->IsSetup(), "Native mesh not yet setup");
					std::pair < VkBuffer, VkDeviceSize> vbuffer;
					vbuffer.first = lastRenderedMesh->vertexBuffer->handle;
					vbuffer.second = 0;
					buffers.push_back(vbuffer);
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffers.back().first, &buffers.back().second);
					vkCmdBindIndexBuffer(commandBuffer, lastRenderedMesh->indexBuffer->handle, 0, VK_INDEX_TYPE_UINT32);
					vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(lastRenderedMesh->GetIndexCount()), 1, 0, 0, 0);
					break;
				}

				case RendererCommand_DrawInstanced:
				{
					TF3D_ASSERT(pipeline, "Cannot draw meshes without a pipeline bound");
					TF3D_ASSERT(param, "Parameter is null");
					std::pair<int, Mesh*>* paramT = reinterpret_cast<std::pair<int, Mesh*>*>(param);
					lastRenderedMesh = reinterpret_cast<Vulkan::NativeMesh*>(paramT->second->GetNativeMesh());
					TF3D_ASSERT(lastRenderedMesh->IsSetup(), "Native mesh not yet setup");
					std::pair < VkBuffer, VkDeviceSize> vbuffer;
					vbuffer.first = lastRenderedMesh->vertexBuffer->handle;
					vbuffer.second = 0;
					buffers.push_back(vbuffer);
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffers.back().first, &buffers.back().second);
					vkCmdBindIndexBuffer(commandBuffer, lastRenderedMesh->indexBuffer->handle, 0, VK_INDEX_TYPE_UINT32);
					vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(lastRenderedMesh->GetIndexCount()), params.num, 0, 0, 0);
					break;
				}

				case RendererCommand_UploadUniform:
				{
					TF3D_ASSERT(pipeline, "Cannot upload uniforms without a pipeline bound");
					if (shader->PushConstantExists(params.str))
					{
						auto& [size, offset] = shader->GetPushConstantLocation(params.str);
						vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, offset, size, param);
					}
					else
					{
						TF3D_LOG_WARN("Uniform {0} not found", params.str);
					}
					break;
				}

				case RendererCommand_CustomFunction:
				{
					void (*func)(void) = reinterpret_cast<void (*)(void)>(param);
					TF3D_ASSERT(func, "Invalid custom function pointer");
					func();
					break;
				}

				case RendererCommand_Push:
				{
					switch ((RendererData)(uint64_t)(param))
					{
					case RendererData_FrameBuffer:
						rendererStack.push(framebuffer);
						break;
					case RendererData_Pipeline:
						rendererStack.push(pipeline);
						break;
						//case RendererData_Camera:
						//	rendererStack.push(camera);
						//	break;
					default:
						TF3D_ASSERT(false, "Unknown RendererData item");
						break;
					}
					break;
				}
				case RendererCommand_Pop:
				{
					TF3D_ASSERT(!rendererStack.empty(), "Renderer stack is empty");
					switch ((RendererData)(uint64_t)(param))
					{
					case RendererData_FrameBuffer:
						framebuffer = reinterpret_cast<Vulkan::FrameBuffer*>(rendererStack.top());
						if (pipeline)
							pipeline->Rebuild(framebuffer);
						break;
					case RendererData_Pipeline:
						pipeline = reinterpret_cast<Vulkan::Pipeline*>(rendererStack.top());
						if (pipeline)
							pipeline->Rebuild(framebuffer);
						break;
						//case RendererData_Camera:
						//	camera = reinterpret_cast<RendererAPI::Camera*>(rendererStack.top());
						//	break;
					default:
						TF3D_ASSERT(false, "Unknown RendererData item");
						break;
					}
					rendererStack.pop();
					break;
				}
				case RendererCommand_PushC:
				{
					rendererStack.push(param);
					break;
				}
				case RendererCommand_PopC:
				{
					rendererStack.pop();
					break;
				}
				default:
					TF3D_ASSERT(false, "Unknown Renderer command");
					break;
				}
			}

			vkCmdEndRenderPass(commandBuffer);
			device->FlushCommandBuffer(commandBuffer);

		}

	}

}

#endif