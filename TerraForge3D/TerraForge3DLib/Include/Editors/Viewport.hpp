#pragma once
#include "Base/Base.hpp"
#include "UI/Editor.hpp"

namespace TerraForge3D
{
	class ApplicationState;

	class Viewport : public UI::Editor
	{
	public:
		Viewport(ApplicationState* appState);
		~Viewport();

		virtual void OnUpdate();
		virtual void OnShow();
		virtual void OnStart();
		virtual void OnEnd();

	protected:
		void RebuildFrameBuffer();

	public:
		uint32_t resolution = 512;

	private:
		ApplicationState* appState = nullptr;
		SharedPtr<RendererAPI::FrameBuffer> framebuffer;
		SharedPtr<RendererAPI::Camera> camera;
	};

}