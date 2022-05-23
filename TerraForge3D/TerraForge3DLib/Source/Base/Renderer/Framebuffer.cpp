#include "Base/Renderer/Framebuffer.hpp"
#include "Base/Vulkan/Framebuffer.hpp"
#include "Base/OpenGL/Framebuffer.hpp"


namespace TerraForge3D
{

	namespace RendererAPI
	{
		FrameBuffer::FrameBuffer()
		{
		}

		FrameBuffer::~FrameBuffer()
		{
		
		}

		FrameBuffer* FrameBuffer::Create()
		{
#ifdef TF3D_VULKAN_BACKEND
			return new Vulkan::FrameBuffer();
#elif defined(TF3D_OPENGL_BACKEND)
			return new OpenGL::FrameBuffer(); 
#endif
		}

	}

}