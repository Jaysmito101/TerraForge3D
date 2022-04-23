#include "Base/Renderer/Context.hpp"
#include "Base/Vulkan/Vulkan.hpp"
#include "Base/OpenGL/OpenGL.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{
		Context* Context::mainInstance = nullptr;

		Context::Context()
		{

		}

		Context::~Context()
		{
		}

		Context* Context::Create()
		{
			TF3D_ASSERT(mainInstance == nullptr, "RendererAPI::Context already created");
#if defined(TF3D_VULKAN_BACKEND)
			mainInstance = new Vulkan::Context();
#elif defined(TF3D_OPENGL_BACKEND)
			mainInstance = new OpenGL::Context();
#endif
			return mainInstance;
		}

		Context* Context::Get()
		{
			TF3D_ASSERT(mainInstance, "RendererAPI::Context not yet created");
			return mainInstance;
		}

		Context* Context::Set(Context* context)
		{
			TF3D_ASSERT(context, "Context is null");
			mainInstance = context;
			return mainInstance;
		}

		void Context::Destroy()
		{
			TF3D_ASSERT(mainInstance, "RendererAPI::Context not yet created");
			TF3D_SAFE_DELETE(mainInstance);
		}

	}

}