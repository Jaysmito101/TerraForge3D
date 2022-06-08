#include "Base/Renderer/NativeRenderer.hpp"
#include "Base/Vulkan/NativeRender.hpp"
#include "Base/OpenGL/NativeRenderer.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{
		NativeRenderer* NativeRenderer::mainInstance = nullptr;


		void NativeRenderer::AddCommand(RendererCommand command, RendererCommandParams params)
		{
			rendererQueue.push(std::make_pair( command, params ));
		}

		NativeRenderer* TerraForge3D::RendererAPI::NativeRenderer::Create()
		{
#ifdef TF3D_OPENGL_BACKEND
			mainInstance = new OpenGL::NativeRenderer();
#elif defined(TF3D_VULKAN_BACKEND)
			mainInstance = new Vulkan::NativeRenderer();
#endif
			return mainInstance;
		}


		void NativeRenderer::Destroy()
		{
			TF3D_SAFE_DELETE(mainInstance);
		}

	}

}