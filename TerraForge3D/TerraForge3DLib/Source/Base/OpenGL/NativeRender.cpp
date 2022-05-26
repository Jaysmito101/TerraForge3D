#include "Base/OpenGL/NativeRenderer.hpp"
#include "Base/Renderer/FrameBuffer.hpp"

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
			RendererAPI::FrameBuffer* framebuffer = nullptr;
			while (!rendererQueue.empty())
			{
				auto& [command, param] = rendererQueue.front();
				rendererQueue.pop();
				switch (command)
				{
				case RendererCommand_Clear:
				{
					TF3D_ASSERT(framebuffer, "Cannot clear without any bound framebuffer");
					float* clearColor = reinterpret_cast<float*>(param);
					framebuffer->Clear(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
					break;
				}
				case RendererCommand_BindFrameBuffer:
				{
					framebuffer = reinterpret_cast<RendererAPI::FrameBuffer*>(param);
					TF3D_ASSERT(framebuffer->IsSetup(), "Framebuffer not yet setup");
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