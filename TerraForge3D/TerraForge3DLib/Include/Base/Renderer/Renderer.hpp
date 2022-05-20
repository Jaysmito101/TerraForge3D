#pragma once
#include "Base/Core/Core.hpp"
#include "Base/Renderer/Context.hpp"
#include "Base/Renderer/UIManager.hpp"

namespace TerraForge3D
{
	namespace RendererAPI
	{
		class NativeRenderer;
	}

	enum RendererCommand
	{
		RendererCommand_None = 0,
		RendererCommand_Clear,
		RendererCommand_Draw,
		RendererCommand_DrawInstanced,
		RendererCommand_BindFrameBuffer,
		RendererCommand_BindPipeline,
		RendererCommand_Render,
		RendererCommand_Count
	};

	class Renderer
	{
	protected:
		Renderer();
		~Renderer();

	public:
		virtual void WaitForIdle();
		virtual void BeginUI();
		virtual void EndUI();

		virtual void Flush();

		static Renderer* Create();
		static Renderer* Get();
		static Renderer* Set(Renderer* context);
		static void Destroy();

	public:
		RendererAPI::Context* rendererContext = nullptr;
		RendererAPI::UIManager* uiManager = nullptr;
		RendererAPI::NativeRenderer* nativeRenderer = nullptr;

	private:
		static Renderer* mainInstance;
	};
}
