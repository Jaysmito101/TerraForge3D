#include "Editors/Viewport.hpp"
#include "TerraForge3D.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

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
		ImGui::BeginChild("Viewport");

		ImVec2 windowSize = ImGui::GetWindowSize();
		ImGui::Image(framebuffer->GetColorAttachmentImGuiID(0), windowSize, ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::IsItemHovered())
		{
			ImVec2 imageBegin = ImGui::GetItemRectMin();
			ImVec2 relativeMousePosition = ImGui::GetMousePos() - imageBegin;
			float mouseX = relativeMousePosition.x / windowSize.x;
			float mouseY = relativeMousePosition.y / windowSize.y;
			float mouseDeltaX = mouseX - prevMouseX;
			float mouseDeltaY = mouseY - prevMouseY;
			prevMouseX = mouseX;
			prevMouseY = mouseY;
			if (fabs(mouseDeltaX) < 10.0f && fabs(mouseDeltaY) < 10.0f)
			{
				if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
				{
					float invertYFactor = invertYInput ? -1.0f : 1.0f;
					// Handle camera position
					if (ImGui::IsKeyDown(ImGuiKey_RightShift) || ImGui::IsKeyDown(ImGuiKey_LeftShift))
					{
						camera->position[0] += mouseDeltaX * positionSpeed;
						camera->position[1] += mouseDeltaY * positionSpeed * invertYFactor;
					}
					// Handle camera rotation
					else
					{
						camera->rotation[1] += mouseDeltaX * rotationSpeed;
						camera->rotation[0] += mouseDeltaY * rotationSpeed * invertYFactor;
					}
				}
			}

		}
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
		camera->lookPosition = focusedPosition;
		this->isVisible = true;
	}

	void Viewport::OnEnd()
	{
		// OPTIONAL : as the following will automatically be done by framebuffers destrustor as autoDestory is true by default
		if (framebuffer->IsSetup())
			framebuffer->Destroy();
	}

	bool Viewport::OnContextMenu()
	{
		ImGui::DragFloat("Camera Movement Speed", &positionSpeed, 0.01f);
		ImGui::DragFloat("Camera Rotation Speed", &rotationSpeed, 0.01f);
		ImGui::Checkbox("Camera Flip Y", &camera->flipY);
		ImGui::Checkbox("Invert Y Input", &invertYInput);

		ImGui::DragFloat3("Camera Position", &camera->position[0], 0.01f);
		ImGui::DragFloat3("Camera Rotation", &camera->rotation[0], 0.01f);

		return ImGui::Button("Close");			
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