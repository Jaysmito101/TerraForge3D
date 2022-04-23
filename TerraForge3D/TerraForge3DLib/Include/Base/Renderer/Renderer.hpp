#pragma once
#include "Base/Core/Core.hpp"
#include "Base/Renderer/Context.hpp"
#include "Base/Renderer/UIManager.hpp"

namespace TerraForge3D
{

	class Renderer
	{
	protected:
		Renderer();
		~Renderer();

	public:
		virtual void WaitForIdle();
		virtual void BeginUI();
		virtual void EndUI();

		static Renderer* Create();
		static Renderer* Get();
		static Renderer* Set(Renderer* context);
		static void Destroy();

	public:
		RendererAPI::Context* rendererContext = nullptr;
		RendererAPI::UIManager* uiManager = nullptr;

	private:
		static Renderer* mainInstance;
	};
}
