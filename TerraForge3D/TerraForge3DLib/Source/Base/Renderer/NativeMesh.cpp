#include "Base/Renderer/NativeMesh.hpp"
#include "Base/Vulkan/NativeMesh.hpp"
#include "Base/OpenGL/NativeMesh.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{
		NativeMesh* NativeMesh::Create()
		{
#ifdef TF3D_OPENGL_BACKEND
			return new OpenGL::NativeMesh();
#elif defined(TF3D_VULKAN_BACKEND)
			return new Vulkan::NativeMesh();
#endif
			return nullptr;
		}

	}

}