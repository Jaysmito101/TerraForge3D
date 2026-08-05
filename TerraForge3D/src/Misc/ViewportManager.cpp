#include "Misc/ViewportManager.h"
#include "Data/ApplicationState.h"
#include "Base/Base.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

#include <string>

ViewportManager::ViewportManager(ApplicationState* appState)
{
	static uint32_t s_ViewportID = 1;
	this->m_ID = s_ViewportID++;
	this->m_IsVisible = false;
	if(this->m_ID == 1) this->m_IsVisible = true; // Show the first viewport by default
	this->m_AppState = appState;
	this->m_RendererViewport = new RendererViewport();
}

ViewportManager::~ViewportManager()
{
	delete this->m_RendererViewport;
}

void ViewportManager::Update()
{
	TF3D_PROFILE_SCOPE(std::string("viewport/") + std::to_string(m_ID) + "/update");
	if (m_AutoCalculateAspectRatio) m_RendererViewport->m_Camera.SetAspectRatio(m_Width / (m_Height + 0.000000001f));
	if (m_IsVisible)
	{
		const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;
		const float scaleX = std::max(framebufferScale.x, 1.0f);
		const float scaleY = std::max(framebufferScale.y, 1.0f);
		constexpr float renderScale = 1.5f;
		const uint32_t renderWidth = static_cast<uint32_t>(std::max(m_Width * scaleX * renderScale, 1.0f));
		const uint32_t renderHeight = static_cast<uint32_t>(std::max(m_Height * scaleY * renderScale, 1.0f));
		{
			TF3D_PROFILE_SCOPE(std::string("viewport/") + std::to_string(m_ID) + "/resize");
			m_RendererViewport->ResizeTo(renderWidth, renderHeight);
		}
		this->m_AppState->rendererManager->Render(this->m_RendererViewport);
	}
	else
	{
		TF3D_PROFILE_SCOPE(std::string("viewport/") + std::to_string(m_ID) + "/hidden");
	}
	m_IsActive &= m_IsVisible;
}

void ViewportManager::Show()
{
	static char s_TempBuffer[1024]; 
	std::sprintf(s_TempBuffer, "Viewport %d", this->m_ID);
	if(!this->m_IsVisible) return;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin(s_TempBuffer, &m_IsVisible);	
	ImGui::PopStyleVar();
	// auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	// auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin();
	ImGui::BeginChild("MainRender");
	if ((m_RendererViewport->m_IsHovered = ImGui::IsWindowHovered()))
	{
		ImGuiIO io = ImGui::GetIO();

		if (m_IsControlEnabled)
		{
			const bool usesCamera = m_RendererViewport->m_ViewportMode == RendererViewportMode_Object ||
				m_RendererViewport->m_ViewportMode == RendererViewportMode_Wireframe;
			if (usesCamera && ImGui::IsKeyPressed(ImGuiKey_Space)) m_RendererViewport->m_Camera.Reset();
			if (io.MouseDown[ImGuiMouseButton_Middle] && (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)))
			{
				if (usesCamera) {
					m_RendererViewport->m_Camera.Pan(io.MouseDelta.x * m_MovementSpeed, io.MouseDelta.y * m_MovementSpeed, m_Height);
				} else
				{
					m_RendererViewport->m_OffsetX -= io.MouseDelta.x * m_MovementSpeed * 0.001f;
					m_RendererViewport->m_OffsetY += io.MouseDelta.y * m_MovementSpeed * 0.001f;
				}
			}
			if (io.MouseDown[ImGuiMouseButton_Middle] && !(ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)))
			{
				if (usesCamera) {
					m_RendererViewport->m_Camera.Orbit(io.MouseDelta.x * m_RotationSpeed, -io.MouseDelta.y * m_RotationSpeed);
				}
				else
				{
					m_RendererViewport->m_OffsetX -= io.MouseDelta.x * m_MovementSpeed * 0.001f;
					m_RendererViewport->m_OffsetY -= io.MouseDelta.y * m_MovementSpeed * 0.001f;
				}
			}
			if (fabs(io.MouseWheel) > 0.000001f)
			{
				if (usesCamera) {
					m_RendererViewport->m_Camera.Zoom(io.MouseWheel * m_ZoomSpeed);
				}
				else
				{
					m_RendererViewport->m_Scale += m_ZoomSpeed * 0.06f * io.MouseWheel;
					m_RendererViewport->m_Scale = glm::clamp(m_RendererViewport->m_Scale, 0.0000001f, 1000000.0f);
				}
			}
			if (usesCamera && ImGui::IsKeyPressed(ImGuiKey_F)) m_RendererViewport->m_Camera.Reset();

			if(ImGui::IsKeyDown(ImGuiKey_Space) && io.MouseDown[ImGuiMouseButton_Right]) ImGui::OpenPopup((std::string("Viewport Settings##") + std::to_string(this->m_ID)).c_str());
		}

		m_RendererViewport->m_MousePosition[0] = m_MousePosX = (ImGui::GetIO().MousePos.x - viewportOffset.x) / m_Width;
		m_RendererViewport->m_MousePosition[1] = m_MousePosY = (ImGui::GetIO().MousePos.y - viewportOffset.y) / m_Height;
		m_RendererViewport->m_MousePosition[1] = 1.0f - m_RendererViewport->m_MousePosition[1];

	}
	else
	{
		m_RendererViewport->m_MousePosition[0] = m_MousePosX = -1.0f;
		m_RendererViewport->m_MousePosition[1] = m_MousePosY = -1.0f;
	}

	ImVec2 imageSize = ImGui::GetWindowSize();
	m_Width = imageSize.x; m_Height = imageSize.y;
	m_RendererViewport->m_AspectRatio = m_Width / (m_Height + 0.000000001f);
	ImGui::Image((ImTextureID)(uint64_t)m_RendererViewport->m_FrameBuffer->GetColorTexture(), imageSize, ImVec2(0, 1), ImVec2(1, 0));
	m_IsActive = ImGui::IsItemHovered();
	ImGui::EndChild();
	this->ShowSettingPopUp();
	ImGui::End();

	m_IsControlEnabled = true; // temp
}

void ViewportManager::ShowSettingPopUp()
{
	if (ImGui::BeginPopupContextItem((std::string("Viewport Settings##") + std::to_string(this->m_ID)).c_str()))
	{
		static const char* viewportModesText[] =
		{
			"Object",
			"Wireframe",
			"Heightmap",
			"TextureSlot"
		};
		
		SHOW_COMBO_BOX("Viewport Mode", m_RendererViewport->m_ViewportMode, viewportModesText, IM_ARRAYSIZE(viewportModesText));
		ImGui::NewLine();

		if (m_RendererViewport->m_ViewportMode != RendererViewportMode_Heightmap && m_RendererViewport->m_ViewportMode != RendererViewportMode_TextureSlot)
		{
			ImGui::Text("Camera Settings");
			ImGui::Separator();
			m_RendererViewport->m_Camera.ShowSettings();
			ImGui::Checkbox("Auto Calculate Aspect Ratio", &m_AutoCalculateAspectRatio);
		}
		else
		{
			ImGui::DragFloat("Offset X", &m_RendererViewport->m_OffsetX, 0.01f);
			ImGui::DragFloat("Offset Y", &m_RendererViewport->m_OffsetY, 0.01f);
			ImGui::DragFloat("Scale", &m_RendererViewport->m_Scale, 0.01f);
		}

		if (m_RendererViewport->m_ViewportMode == RendererViewportMode_TextureSlot)
		{
			ImGui::Checkbox("Texture Slot Detailed Mode", &m_RendererViewport->m_TextureSlotDetailedMode);
			if (m_RendererViewport->m_TextureSlotDetailedMode)
			{
				static const char* s_TextureSlotChannels[] = { "R", "G", "B", "A" };
				for (int i = 0; i < 4; i++)
				{
					ImGui::PushID(i);
					ImGui::Text("Viewport Channel %s :", s_TextureSlotChannels[i]);
					if(ImGui::DragInt("Texture Slot", &m_RendererViewport->m_TextureSlotDetailed[i].first, 0.1f, 0, 5))
						m_RendererViewport->m_TextureSlotDetailed[i].first = glm::clamp(m_RendererViewport->m_TextureSlotDetailed[i].first, 0, 5);
					ShowTextureSlotDetailsPopup();
					if (ImGui::DragInt("Texture Slot Channel", &m_RendererViewport->m_TextureSlotDetailed[i].second, 0.1f, 0, 3))
						m_RendererViewport->m_TextureSlotDetailed[i].first = glm::clamp(m_RendererViewport->m_TextureSlotDetailed[i].first, 0, 3);
					ImGui::PopID();
				}

			}
			else
			{
				if (ImGui::DragInt("Texture Slot", &m_RendererViewport->m_TextureSlot, 0.1f, 0, 5))
					m_RendererViewport->m_TextureSlot = glm::clamp(m_RendererViewport->m_TextureSlot, 0, 5);
				ShowTextureSlotDetailsPopup();
			}
		}
		ImGui::Text("Speed Settings");
		ImGui::DragFloat("Movement Speed", &m_MovementSpeed, 0.01f);
		ImGui::DragFloat("Rotation Speed", &m_RotationSpeed, 0.01f);
		ImGui::DragFloat("Zoom Speed", &m_ZoomSpeed, 0.01f);
		ImGui::NewLine();


		ImGui::EndPopup();
	}
}

void ViewportManager::ShowTextureSlotDetailsPopup()
{
	if (ImGui::BeginPopupContextItem())
	{
		ImGui::Text("0 : [Heightmap, Custom, Custom, Custom]");
		ImGui::Text("1 : [Custom, Custom, Custom, Custom]");
		ImGui::Text("2 : [Custom, Custom, Custom, Custom]");
		ImGui::Text("3 : Albedo");
		ImGui::Text("4 : Normal");
		ImGui::Text("5 : [AO, Roughness, Metallic, Unused]");
		ImGui::EndPopup();
	}
}
