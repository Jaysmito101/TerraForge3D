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

	class Mesh;

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
		RendererCommand_UploadUniform,
		RendererCommand_CustomFunction,
		RendererCommand_Push,
		RendererCommand_Pop,
		RendererCommand_PushC,
		RendererCommand_PopC,
		RendererCommand_Count
		//RendererCommand_BindCamera,
	};

	struct RendererCommandParams
	{
		void* custom = nullptr;
		std::string str = "";
		int32_t num = 0;
	};

	class Renderer
	{
	protected:
		Renderer();
		virtual ~Renderer();

	public:
		virtual void WaitForIdle();
		virtual void BeginUI();
		virtual void EndUI();

		virtual void Flush();
		
		inline Renderer* SetClearColor(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) { this->clearColor[0] = r; this->clearColor[1] = g; this->clearColor[2] = b;	this->clearColor[3] = a; return this; }
		inline Renderer* SetCamera(RendererAPI::Camera* camera) { this->camera = camera; return this; }

		Renderer* ClearFrame();
		Renderer* BindFramebuffer(RendererAPI::FrameBuffer* framebuffer);
		Renderer* BindPipeline(RendererAPI::Pipeline* pipeline);

		Renderer* UploadUnifrom(std::string name, void* data);

		Renderer* DrawMesh(Mesh* mesh, int32_t mousePickID = 0);

		Renderer* CustomFunction(void (*func)(void));

		static Renderer* Create();
		static Renderer* Get();
		static Renderer* Set(Renderer* context);
		static void Destroy();

	public:
		RendererAPI::Context* rendererContext = nullptr;
		RendererAPI::UIManager* uiManager = nullptr;
		RendererAPI::NativeRenderer* nativeRenderer = nullptr;
		RendererAPI::Camera* camera = nullptr;

	private:
		static Renderer* mainInstance;

		float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	};
}
