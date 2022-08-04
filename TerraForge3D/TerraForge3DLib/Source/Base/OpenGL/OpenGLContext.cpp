#include "Base/OpenGL/Context.hpp"

#ifdef TF3D_OPENGL_BACKEND

namespace TerraForge3D
{
	namespace OpenGL
	{

		Context::Context()
		{
			if (!gladLoadGL())
			{
				TF3D_LOG_ERROR("Failed to load OpenGL functions");
				exit(-1);
			}
		}

		Context::~Context()
		{
			
		}

		void Context::WaitForIdle()
		{
			// TODO: Implement
		}

		std::string GetGPUName()
		{
			const char* vendor = (const char*)glGetString(GL_VENDOR);
			const char* renderer = (const char*)glGetString(GL_RENDERER);
			return std::string(vendor) + " " + std::string(renderer);
		}
	}

}

#endif