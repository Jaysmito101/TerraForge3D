#include "Base/Renderer/Shader.hpp"
#include "Base/OpenGL/Shader.hpp"
#include "Base/Vulkan/Shader.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{

		Shader* Shader::Create()
		{
#ifdef TF3D_OPENGL_BACKEND
			return new OpenGL::Shader();
#elif defined(TF3D_VULKAN_BACKEND)
			return new  Vulkan::Shader();
#endif
			return nullptr;
		}

	}

}