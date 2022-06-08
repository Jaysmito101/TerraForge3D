#include "Base/Renderer/Renderer.hpp"
#include "Base/Base.hpp"

namespace TerraForge3D
{
	Renderer* Renderer::mainInstance = nullptr;

	class Mesh;

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
		rendererContext->WaitForIdle();
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

	Renderer* Renderer::ClearFrame()
	{
		nativeRenderer->AddCommand(RendererCommand_Clear, clearColor);
		return this;
	}

	Renderer* Renderer::BindFramebuffer(RendererAPI::FrameBuffer* framebuffer)
	{
		nativeRenderer->AddCommand(RendererCommand_BindFrameBuffer, framebuffer);
		return this;
	}

	Renderer* Renderer::BindPipeline(RendererAPI::Pipeline* pipeline)
	{
		nativeRenderer->AddCommand(RendererCommand_BindPipeline, pipeline);
		if (this->camera)
		{
			this->camera->RecalculateMatrices();
			RendererCommandParams params;
			params.custom = glm::value_ptr(camera->matrices.pv);
			params.str = "_PV";
			nativeRenderer->AddCommand(RendererCommand_UploadUniform, params);
		}
		return this;
	}

	Renderer* Renderer::DrawMesh(Mesh* mesh)
	{	
		RendererCommandParams params;
		params.custom = glm::value_ptr(mesh->GetModelMatrix());
		params.str = "_Model";
		nativeRenderer->AddCommand(RendererCommand_UploadUniform, params);
		nativeRenderer->AddCommand(RendererCommand_Draw, mesh);
		return this;
	}

	Renderer* Renderer::CustomFunction(void (*func)(void))
	{
		nativeRenderer->AddCommand(RendererCommand_CustomFunction, func);
		return this;
	}
}