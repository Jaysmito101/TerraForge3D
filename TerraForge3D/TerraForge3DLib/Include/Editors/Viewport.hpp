#pragma once
#include "Base/Base.hpp"
#include "UI/Editor.hpp"

namespace TerraForge3D
{
	class ApplicationState;

	enum ViewportMode
	{
		ViewportMode_Object = 0,
		ViewportMode_Wireframe,
		ViewportMode_Rendererd
	};

	static const char* ViewportModeStr[] = {
		"Object",
		"Wireframe",
		"Rendererd"
	};

	class Viewport : public UI::Editor
	{
	public:
		Viewport(ApplicationState* appState, uint32_t number = 1);
		~Viewport();

		virtual void OnUpdate() override;
		virtual void OnShow() override;
		virtual void OnStart() override;
		virtual void OnEnd() override;
		virtual bool OnContextMenu() override;

		inline Viewport* SetVisible(bool value) { this->isVisible = value; return this; }

		inline SharedPtr<RendererAPI::Camera>& GetCamera() { return this->camera; }
		inline SharedPtr<RendererAPI::FrameBuffer>& GetFramebuffer() { return this->framebuffer; }
		inline ViewportMode GetMode() { return this->mode; }
		inline uint32_t GetNumber() { return this->viewportNumber; }

	protected:
		void RebuildFrameBuffer();

	public:
		uint32_t resolution = 512;

	private:
		ApplicationState* appState = nullptr;
		SharedPtr<RendererAPI::FrameBuffer> framebuffer;
		SharedPtr<RendererAPI::Camera> camera;
		float prevMouseX = 0.0f;
		float prevMouseY = 0.0f;
		float rotationSpeed = 80.0f;
		float positionSpeed = 4.0f;
		float zoomSpeed = 0.1f;
		bool invertYInput = false;
		uint32_t viewportNumber = 1;
		bool isHovered = false;
		ViewportMode mode = ViewportMode_Object;
		uint32_t mouseScrollEventCallback = 0;
		glm::vec3 focusedPosition = glm::vec3(0.0f);
	};

}