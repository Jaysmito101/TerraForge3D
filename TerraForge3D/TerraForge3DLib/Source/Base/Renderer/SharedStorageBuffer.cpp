#include "Base/Renderer/SharedStorageBuffer.hpp"
#include "Base/OpenGL/SharedStorageBuffer.hpp"
// #include "Base/Vulkan/SharedStorageBuffer.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{
		SharedStorageBuffer* SharedStorageBuffer::Create()
		{
#ifdef TF3D_OPENGL_BACKEND
			return new OpenGL::SharedStorageBuffer();
#elif defined(TF3D_VULKAN_BACKEND)
			return new Vulkan::SharedStorageBuffer();
#endif
		}
	}

}