#include "Editors/Viewport.hpp"
#include "TerraForge3D.hpp"

namespace TerraForge3D
{
	Viewport::Viewport(ApplicationState* appState)
	{
		this->appState = appState;
	}

	Viewport::~Viewport()
	{
	}

	void Viewport::OnUpdate()
	{
		appState->renderer->SetCamera(camera.Get());

		appState->renderer->BindFramebuffer(framebuffer.Get());
		appState->renderer->SetClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		appState->renderer->ClearFrame();
		appState->renderer->BindPipeline(appState->pipeline.Get());
		appState->renderer->DrawMesh(appState->mesh.Get());
	}

	void Viewport::OnShow()
	{
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		ImGui::BeginChild("Viewport");

		ImVec2 windowSize = ImGui::GetWindowSize();
		ImGui::Image(framebuffer->GetColorAttachmentImGuiID(0), windowSize, ImVec2(0, 1), ImVec2(1, 0));

		if (windowSize.x != 0.0f && windowSize.y != 0.0f)
			camera->SetPerspective(90.0f, windowSize.x / windowSize.y, 0.01f, 1000.0f);
		
		ImGui::EndChild();
	}

	void Viewport::OnStart()
	{
		framebuffer = RendererAPI::FrameBuffer::Create();
		RebuildFrameBuffer();
		camera = new RendererAPI::Camera();
		camera->SetPerspective(90.0f, 1.0f, 0.01f, 1000.0f);
		camera->SetPosition(0.0, 0.0, -1.f);
		camera->SetRotation(0.0, 0.0, 0.0);

		this->isVisible = true;
	}

	void Viewport::OnEnd()
	{
		// OPTIONAL : as the following will automatically be done by framebuffers destrustor as autoDestory is true by default
		if (framebuffer->IsSetup())
			framebuffer->Destroy();
	}

	void Viewport::RebuildFrameBuffer()
	{
		if (framebuffer->IsSetup())
			framebuffer->Destroy();
		framebuffer->SetSize(resolution, resolution);
		framebuffer->SetColorAttachmentCount(1);
		framebuffer->SetDepthAttachment(true);
		framebuffer->Setup();
	}
}