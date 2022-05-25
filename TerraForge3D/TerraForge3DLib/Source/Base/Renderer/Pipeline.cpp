#include "Base/Renderer/Pipeline.hpp"
#include "Base/OpenGL/Pipeline.hpp"
#include "Base/Vulkan/Pipeline.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{

		Pipeline* Pipeline::Create()
		{
#ifdef TF3D_OPENGL_BACKEND
			return new OpenGL::Pipeline();
#elif defined(TF3D_VULKAN_BACKEND
			return new Vulkan::Pipeline();
#endif
			return nullptr;
		}

	}

}