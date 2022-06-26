#include "Base/OpenGL/NativeRenderer.hpp"
#include "Base/OpenGL/FrameBuffer.hpp"
#include "Base/OpenGL/Pipeline.hpp"
#include "Base/OpenGL/NativeMesh.hpp"
#include "Base/OpenGL/Shader.hpp"
#include "Base/Renderer/Camera.hpp"

#include "Base/DS/Mesh.hpp"

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
			OpenGL::Shader* shader = nullptr;
			RendererAPI::Camera* camera = nullptr;

			GLuint mousePickIDUniformLocation = 0;

			std::stack<void*> rendererStack;


			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS); 

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
					glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					break;
				}
				case RendererCommand_BindFrameBuffer:
				{
					TF3D_ASSERT(param, "Parameter is null");
					framebuffer = reinterpret_cast<OpenGL::FrameBuffer*>(param);
					TF3D_ASSERT(framebuffer->IsSetup(), "Framebuffer not yet setup");
					// if (pipeline)
					//	pipeline->Rebuild(framebuffer);
					glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->handle);
					break;
				}
				case RendererCommand_BindPipeline:
				{
					TF3D_ASSERT(param, "Parameter is null");
					TF3D_ASSERT(framebuffer, "Binding renderer pipeline without a framebuffer bound");
					pipeline = reinterpret_cast<OpenGL::Pipeline*>(param);
					TF3D_ASSERT(pipeline->IsSetup(), "Pipeline is not yet setup");
					shader = reinterpret_cast<OpenGL::Shader*>(pipeline->shader);
					mousePickIDUniformLocation = glGetUniformLocation(shader->handle, "_MousePickID"); // TEMP here
					TF3D_ASSERT(shader, "Shader is null");
					TF3D_ASSERT(shader->IsCompiled(), "Shader is not compiled");
					pipeline->Rebuild(framebuffer);
					glUseProgram(shader->handle);
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
					lastRendererMesh = reinterpret_cast<OpenGL::NativeMesh*>(mesh->GetNativeMesh());
					TF3D_ASSERT(lastRendererMesh->IsSetup(), "Native mesh not yet setup");
					glUniform1i(mousePickIDUniformLocation, params.num);
						//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					glBindVertexArray(lastRendererMesh->vertexArray);
					glDrawElements(GL_TRIANGLES, (GLsizei)lastRendererMesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
					glPolygonMode(GL_BACK, GL_LINE); glBindVertexArray(0);
						//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					break;
				}

				case RendererCommand_DrawInstanced:
				{
					TF3D_ASSERT(pipeline, "Cannot draw meshes without a pipeline bound");
					TF3D_ASSERT(param, "Parameter is null");
					std::pair<int, Mesh*>* paramT = reinterpret_cast<std::pair<int, Mesh*>*>(param);
					lastRendererMesh = reinterpret_cast<OpenGL::NativeMesh*>(paramT->second->GetNativeMesh());
					TF3D_ASSERT(lastRendererMesh->IsSetup(), "Native mesh not yet setup");
					glUniform1i(mousePickIDUniformLocation, params.num); 
					glBindVertexArray(lastRendererMesh->vertexArray);
					glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)lastRendererMesh->GetIndexCount(), GL_UNSIGNED_INT, 0, paramT->first);
					glBindVertexArray(0);
					break;
				}

				case RendererCommand_UploadUniform:
				{
					TF3D_ASSERT(pipeline, "Cannot upload uniforms without a pipeline bound");
					if (shader->UniformExists(params.str))
					{
						auto& [location, type] = shader->GetUniform(params.str);
						switch (type)
						{
							case RendererAPI::ShaderDataType_Mat3:
								glUniformMatrix3fv(location, 1, GL_FALSE, (GLfloat*)params.custom);
								break;
							case RendererAPI::ShaderDataType_Mat4:
								glUniformMatrix4fv(location, 1, GL_FALSE, (GLfloat*)params.custom);
								break;
							case RendererAPI::ShaderDataType_Vec2:
								glUniform2fv(location, 1, (GLfloat*)params.custom);
								break;
							case RendererAPI::ShaderDataType_IVec2:
								glUniform2iv(location, 1, (GLint*)params.custom);
								break;
							case RendererAPI::ShaderDataType_Vec3:
								glUniform3fv(location, 1, (GLfloat*)params.custom);
								break;
							case RendererAPI::ShaderDataType_IVec3:
								glUniform3iv(location, 1, (GLint*)params.custom);
								break;
							case RendererAPI::ShaderDataType_Vec4:
								glUniform4fv(location, 1, (GLfloat*)params.custom);
								break;
							case RendererAPI::ShaderDataType_IVec4:
								glUniform4iv(location, 1, (GLint*)params.custom);
								break;
							case RendererAPI::ShaderDataType_Bool:
								glUniform1i(location, (GLint)params.custom);
								break;
						}
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
						framebuffer = reinterpret_cast<OpenGL::FrameBuffer*>(rendererStack.top());
						if (pipeline)
							pipeline->Rebuild(framebuffer);
						break;
					case RendererData_Pipeline:
						pipeline = reinterpret_cast<OpenGL::Pipeline*>(rendererStack.top());
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
		}

	}

}

#endif