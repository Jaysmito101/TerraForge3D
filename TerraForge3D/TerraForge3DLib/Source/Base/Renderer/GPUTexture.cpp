#include "Base/Renderer/GPUTexture.hpp"

#include "Base/Vulkan/GPUTexture.hpp"
#include "Base/OpenGL/GPUTexture.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{



		GPUTexture::GPUTexture()
		{
		}

		GPUTexture::~GPUTexture()
		{
		}

		GPUTexture* GPUTexture::Create()
		{
#if defined(TF3D_OPENGL_BACKEND)
			return new OpenGL::GPUTexture();
#elif defined(TF3D_VULKAN_BACKEND)
			Vulkan::GPUTexture* tex = new Vulkan::GPUTexture();
			tex->UseGraphicsDevice();
			return tex;
#endif
		}

	}

}
