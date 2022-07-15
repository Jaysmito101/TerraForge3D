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

	}

}

#endif