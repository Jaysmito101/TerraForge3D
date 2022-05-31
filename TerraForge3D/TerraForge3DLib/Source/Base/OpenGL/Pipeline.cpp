#include "Base/OpenGL/Pipeline.hpp"
#include "Base/OpenGL/FrameBuffer.hpp"

#ifdef TF3D_OPENGL_BACKEND

namespace TerraForge3D
{

	namespace OpenGL
	{
		Pipeline::Pipeline()
		{
		}

		Pipeline::~Pipeline()
		{
		}

		void Pipeline::Setup()
		{
			isSetup = true;
		}

		void Pipeline::Destory()
		{
			isSetup = false;
		}

		bool Pipeline::Rebuild(RendererAPI::FrameBuffer* fbo)
		{
			if (this->framebuffer != fbo)
			{
				this->framebuffer = reinterpret_cast<OpenGL::FrameBuffer*>(fbo);
				glViewport(viewportBegin[0], viewportBegin[1], framebuffer->GetWidth(), framebuffer->GetHeight());
			}
			return true;
		}

	}

}

#endif