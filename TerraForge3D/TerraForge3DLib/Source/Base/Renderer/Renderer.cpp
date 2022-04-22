#include "Base/Renderer/Renderer.hpp"
#include "Base/Renderer/OpenGLRenderer.hpp"
#include "Base/Renderer/VulkanRenderer.hpp"

namespace TerraForge3D
{
	Renderer* Renderer::mainInstance = nullptr;

	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}

	Renderer* Renderer::Create()
	{
		TF3D_ASSERT(mainInstance == nullptr, "Renderer already created");
#if defined(TF3D_OPENGL_BACKEND)
		mainInstance = new OpenGLRenderer();
#elif defined(TF3D_VULKAN_BACKEND)
		mainInstance = new VulkanRenderer();
#else
#error "Unknown Renderer Backend"
#endif
		return mainInstance;
	}

	Renderer* Renderer::Get()
	{
		TF3D_ASSERT(mainInstance, "Renderer not yet initialized");
		return mainInstance;
	}

	Renderer* Renderer::Set(Renderer* context)
	{
		TF3D_ASSERT(context, "context is null");
		mainInstance = context;
		return mainInstance;
	}

	void Renderer::Destroy()
	{
		TF3D_ASSERT(mainInstance, "Renderer not yet initialized");
		mainInstance->Shutdown();
		TF3D_SAFE_DELETE(mainInstance);
	}

}