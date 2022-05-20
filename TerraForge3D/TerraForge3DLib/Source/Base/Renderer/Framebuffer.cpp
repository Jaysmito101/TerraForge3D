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

		void FrameBuffer::Destroy(FrameBuffer* framebuffer)
		{
			TF3D_ASSERT(framebuffer, "Cannot destroy null framebuffer");
			TF3D_SAFE_DELETE(framebuffer);
		}

	}

}