#include "Editors/Viewport.hpp"
#include "TerraForge3D.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

namespace TerraForge3D
{
	Viewport::Viewport(ApplicationState* appState, uint32_t number)
	:UI::Editor("Viewport " + std::to_string(number)), viewportNumber(number)
	{
		this->appState = appState;
	}

	Viewport::~Viewport()
	{
	}

	void Viewport::OnUpdate()
	{
		// Rendeer the frame and update mouse only if the viewport is visible
		if (isVisible)
		{
			appState->editors.inspector->RenderItems(this);
			uint32_t mx = static_cast<uint32_t>(prevMouseX * (resolution - 2)) + 1;
			uint32_t my = resolution - static_cast<uint32_t>(prevMouseY * (resolution - 2)) + 1;
			//int32_t val = 0;
			//framebuffer->ReadPixel(mx, my, 1, &val);
			//TF3D_LOG("{} {} -> {}", mx, my, val);

		}
	}

	void Viewport::OnShow()
	{
		ImGui::BeginChild("Viewport");

		ImVec2 windowSize = ImGui::GetWindowSize();
		ImGui::Image(framebuffer->GetColorAttachmentImGuiID(0), windowSize, ImVec2(0, 1), ImVec2(1, 0));
		isHovered = ImGui::IsItemHovered();
		if (isHovered)
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
						camera->rotation[0] += mouseDeltaY * rotationSpeed * invertYFactor * -1.0f;
					}
				}
			}

		}
		if (windowSize.x != 0.0f && windowSize.y != 0.0f)
			camera->aspect = windowSize.x / windowSize.y;
		
		ImGui::EndChild();
	}

	void Viewport::OnStart()
	{
		framebuffer = RendererAPI::FrameBuffer::Create();
		RebuildFrameBuffer();
		camera = new RendererAPI::Camera();
		camera->SetPerspective(45.0f, 1.0f, 0.01f, 1000.0f);
		camera->SetPosition(0.0, 0.0, -1.f);
		camera->SetRotation(0.0, 0.0, 0.0);
		
		appState->menus.mainMenu->GetManagerPTR()->Register("Windows/Viewports/Viewport " + std::to_string(viewportNumber), [this](TerraForge3D::UI::MenuItem* context) {
			isVisible = (context->GetToggleState());
			}, TerraForge3D::UI::MenuItemUse_Toggle)->RegisterTogglePTR(&isVisible);
		
		isVisible = false;

		mouseScrollEventCallback = appState->core.window->eventManager->RegisterCallback([this](MouseScrollEventParams* params) -> bool {
			if (this->isHovered)
			{
				float scrollDelta = params->offsetY;
				// TEMP BEGIN
				// This part will be modified later	
				this->camera->position += glm::vec3(0, 0, 1) * this->zoomSpeed * scrollDelta;
				// TEMP END
			}
			return true;
		});
	}

	void Viewport::OnEnd()
	{
		// OPTIONAL : as the following will automatically be done by framebuffers destrustor as autoDestory is true by default
		if (framebuffer->IsSetup())
			framebuffer->Destroy();

		appState->core.window->eventManager->DeregisterCallback(mouseScrollEventCallback);
	}

	bool Viewport::OnContextMenu()
	{
		ImGui::DragFloat("Camera Movement Speed", &positionSpeed, 0.01f);
		ImGui::DragFloat("Camera Rotation Speed", &rotationSpeed, 0.01f);
		ImGui::DragFloat("Camera Zoom Speed", &zoomSpeed, 0.01f);
		ImGui::DragFloat("Camera FOV", &camera->fov, 0.01f);
		ImGui::Checkbox("Camera Flip Y", &camera->flipY);
		ImGui::Checkbox("Invert Y Input", &invertYInput);

		ImGui::DragFloat3("Camera Position", &camera->position[0], 0.01f);
		ImGui::DragFloat3("Camera Rotation", &camera->rotation[0], 0.01f);

		if (Utils::ImGuiC::ComboBox("Viewport Mode", &mode, ViewportModeStr, TF3D_STATIC_ARRAY_SIZE(ViewportModeStr)))
		{
			TF3D_LOG_TRACE("Viewport Mode Changed for Viewport {}", viewportNumber);
		}

		return ImGui::Button("Close");			
	}

	void Viewport::RebuildFrameBuffer()
	{
		if (framebuffer->IsSetup())
			framebuffer->Destroy();
		framebuffer->SetSize(resolution, resolution);
		framebuffer->SetColorAttachmentCount(2);
		framebuffer->SetColorAttachmentFormat(RendererAPI::FrameBufferAttachmentFormat_RGBA16, 0);
		framebuffer->SetColorAttachmentFormat(RendererAPI::FrameBufferAttachmentFormat_R32I, 1);
		framebuffer->SetDepthAttachment(true);
		framebuffer->Setup();
	}
}