#include "Base/OpenGL/Pipeline.hpp"

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

	}

}

#endif