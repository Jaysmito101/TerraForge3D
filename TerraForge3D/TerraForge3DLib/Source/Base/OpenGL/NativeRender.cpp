#include "Base/OpenGL/NativeRenderer.hpp"
#include "Base/OpenGL/FrameBuffer.hpp"
#include "Base/OpenGL/Pipeline.hpp"
#include "Base/OpenGL/NativeMesh.hpp"
#include "Base/Renderer/Camera.hpp"

#include <stack>

#ifdef TF3D_OPENGL_BACKEND

namespace TerraForge3D
{

	namespace OpenGL
	{

		NativeRenderer::NativeRenderer()
		{
			
		}

		NativeRenderer::~NativeRenderer()
		{
		}

		void NativeRenderer::Flush()
		{
			OpenGL::FrameBuffer* framebuffer = nullptr;
			OpenGL::Pipeline* pipeline = nullptr;
			OpenGL::NativeMesh* lastRendererMesh = nullptr;
			RendererAPI::Camera* camera = nullptr;

			std::stack<void*> rendererStack;

			while (!rendererQueue.empty())
			{
				auto& [command, param] = rendererQueue.front();
				rendererQueue.pop();
				switch (command)
				{
				case RendererCommand_Clear:
				{
					TF3D_ASSERT(param, "Parameter is null");
					TF3D_ASSERT(framebuffer, "Cannot clear without any bound framebuffer");
					float* clearColor = reinterpret_cast<float*>(param);
					framebuffer->Clear(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
					break;
				}
				case RendererCommand_BindFrameBuffer:
				{
					TF3D_ASSERT(param, "Parameter is null");
					framebuffer = reinterpret_cast<OpenGL::FrameBuffer*>(param);
					TF3D_ASSERT(framebuffer->IsSetup(), "Framebuffer not yet setup");
					// if (pipeline)
					//	pipeline->Rebuild(framebuffer);
					break;
				}
				case RendererCommand_BindPipeline:
				{
					TF3D_ASSERT(param, "Parameter is null");
					if (!framebuffer)
					{
						TF3D_LOG_WARN("Binding renderer pipeline without a framebuffer bound");
					}
					pipeline = reinterpret_cast<OpenGL::Pipeline*>(param);
					TF3D_ASSERT(pipeline->IsSetup(), "Pipeline is not yet setup");
					// if (!pipeline->IsReady())
					//   pipeline->Rebuild(framebuffer);
					break;
				}
				case RendererCommand_BindCamera:
				{
					TF3D_ASSERT(param, "Parameter is null");
					if (!pipeline)
					{
						TF3D_LOG_WARN("Binding camera without a pipeline bound");
					}
					camera = reinterpret_cast<RendererAPI::Camera*>(param);
					camera->RecalculateMatrices();
					break;
				}

				case RendererCommand_Draw:
				{
					TF3D_ASSERT(param, "Parameter is null");
					lastRendererMesh = reinterpret_cast<OpenGL::NativeMesh*>(param);
					TF3D_ASSERT(lastRendererMesh->IsSetup(), "Native mesh not yet setup");
					glBindVertexArray(lastRendererMesh->vertexArray);
					glDrawElements(GL_TRIANGLES, lastRendererMesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
					glBindVertexArray(0);
					break;
				}

				case RendererCommand_DrawInstanced:
				{
					TF3D_ASSERT(param, "Parameter is null");
					std::pair<int, OpenGL::NativeMesh*>* paramT = reinterpret_cast<std::pair<int, OpenGL::NativeMesh*>*>(param);
					lastRendererMesh = paramT->second;
					TF3D_ASSERT(lastRendererMesh->IsSetup(), "Native mesh not yet setup");
					glBindVertexArray(lastRendererMesh->vertexArray);
					glDrawElementsInstanced(GL_TRIANGLES, lastRendererMesh->GetIndexCount(), GL_UNSIGNED_INT, 0, paramT->first);
					glBindVertexArray(0);
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
					case RendererData_Camera:
						rendererStack.push(camera);
						break;
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
						framebuffer = reinterpret_cast<OpenGL::FrameBuffer*>(rendererStack.top());
						break;
					case RendererData_Pipeline:
						pipeline = reinterpret_cast<OpenGL::Pipeline*>(rendererStack.top());
						break;
					case RendererData_Camera:
						camera = reinterpret_cast<RendererAPI::Camera*>(rendererStack.top());
						break;
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
		}

	}

}

#endif