#pragma once
#include "Base/Core/Core.hpp"
#include "Base/Renderer/Context.hpp"
#include "Base/Renderer/UIManager.hpp"


namespace TerraForge3D
{
	namespace RendererAPI
	{
		class NativeRenderer;
		class FrameBuffer;
		class Pipeline;
		class NativeMesh;
		class Camera;
	}

	enum RendererData
	{
		RendererData_FrameBuffer = 0,
		RendererData_Pipeline,
		RendererData_Camera,
		RendererData_Count
	};

	enum RendererCommand
	{
		RendererCommand_None = 0,
		RendererCommand_Clear,
		RendererCommand_Draw,
		RendererCommand_DrawInstanced,
		RendererCommand_BindFrameBuffer,
		RendererCommand_BindPipeline,
		RendererCommand_BindCamera,
		RendererCommand_Push,
		RendererCommand_Pop,
		RendererCommand_PushC,
		RendererCommand_PopC,
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
		
		inline Renderer* SetClearColor(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) { this->clearColor[0] = r; this->clearColor[1] = g; this->clearColor[2] = b;	this->clearColor[3] = a; return this; }

		Renderer* ClearFrame();
		Renderer* BindFramebuffer(RendererAPI::FrameBuffer* framebuffer);
		Renderer* BindPipeline(RendererAPI::Pipeline* pipeline);
		Renderer* BindCamera(RendererAPI::Camera* camera);

		Renderer* DrawMesh(RendererAPI::NativeMesh* mesh);

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

		float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	};
}
