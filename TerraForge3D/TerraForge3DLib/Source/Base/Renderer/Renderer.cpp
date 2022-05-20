#include "Base/Renderer/Renderer.hpp"
#include "Base/Renderer/NativeRenderer.hpp"

namespace TerraForge3D
{
	Renderer* Renderer::mainInstance = nullptr;


	Renderer* Renderer::Create()
	{ 
		TF3D_ASSERT(mainInstance == nullptr, "Renderer already created");
		mainInstance = new Renderer();
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
		mainInstance->WaitForIdle();
		TF3D_SAFE_DELETE(mainInstance);
	}

	Renderer::Renderer()
	{
		rendererContext = RendererAPI::Context::Create();
		uiManager = RendererAPI::UIManager::Create();
		nativeRenderer = RendererAPI::NativeRenderer::Create();
	}

	Renderer::~Renderer()
	{
		RendererAPI::NativeRenderer::Destroy();
		RendererAPI::UIManager::Destory();
		RendererAPI::Context::Destroy();		
	}

	void Renderer::WaitForIdle()
	{
		rendererContext->WaitForIdle();
	}

	void Renderer::BeginUI()
	{
		uiManager->Begin();
	}

	void Renderer::EndUI()
	{
		uiManager->End();
	}

	void Renderer::Flush()
	{
		nativeRenderer->Flush();
	}

	
}