#include "Misc/ViewportManager.h"
#include "Data/ApplicationState.h"
#include "Base/Base.h"
#include "Utils/Utils.h"

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
	if (m_AutoCalculateAspectRatio) m_RendererViewport->m_Camera.aspect = (m_Width / (m_Height + 0.000000001f));
	if (m_IsVisible) this->m_AppState->rendererManager->Render(this->m_RendererViewport);
}

void ViewportManager::Show()
{
	static char s_TempBuffer[1024]; 
	std::sprintf(s_TempBuffer, "Viewport %d", this->m_ID);
	if(!this->m_IsVisible) return;
	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();
	ImGui::Begin(s_TempBuffer, &m_IsVisible);	
	ImGui::BeginChild("MainRender");
	if (ImGui::IsWindowHovered())
	{
		ImGuiIO io = ImGui::GetIO();

		if (io.MouseDown[ImGuiMouseButton_Middle] && (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)))
		{
			m_RendererViewport->m_Camera.position[0] += m_MovementSpeed * (- io.MouseDelta.x * 0.005f * glm::distance(glm::vec3(0.0f), glm::vec3(m_RendererViewport->m_Camera.position[0], m_RendererViewport->m_Camera.position[1], m_RendererViewport->m_Camera.position[2])));
			m_RendererViewport->m_Camera.position[1] += m_MovementSpeed * (io.MouseDelta.y * 0.005f * glm::distance(glm::vec3(0.0f), glm::vec3(m_RendererViewport->m_Camera.position[0], m_RendererViewport->m_Camera.position[1], m_RendererViewport->m_Camera.position[2])));
			m_RendererViewport->m_OffsetX -= io.MouseDelta.x * m_MovementSpeed * 0.001f;
			m_RendererViewport->m_OffsetY += io.MouseDelta.y * m_MovementSpeed * 0.001f;
		}
		if (io.MouseDown[ImGuiMouseButton_Middle] && !(ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)))
		{
			m_RendererViewport->m_Camera.rotation[0] += io.MouseDelta.x * m_RotationSpeed * 10.0f;
			m_RendererViewport->m_Camera.rotation[1] += io.MouseDelta.y * m_RotationSpeed * 10.0f;
			m_RendererViewport->m_OffsetX -= io.MouseDelta.x * m_MovementSpeed * 0.001f;
			m_RendererViewport->m_OffsetY += io.MouseDelta.y * m_MovementSpeed * 0.001f;
		}
		if (fabs(io.MouseWheel) > 0.000001f)
		{
			m_RendererViewport->m_Camera.position[0] += m_AppState->constants.FRONT.x * m_ZoomSpeed * 0.06f * io.MouseWheel;
			m_RendererViewport->m_Camera.position[1] += m_AppState->constants.FRONT.y * m_ZoomSpeed * 0.06f * io.MouseWheel;
			m_RendererViewport->m_Camera.position[2] += m_AppState->constants.FRONT.z * m_ZoomSpeed * 0.06f * io.MouseWheel;
			m_RendererViewport->m_Scale += m_ZoomSpeed * 0.06f * io.MouseWheel;
			m_RendererViewport->m_Scale = glm::clamp(m_RendererViewport->m_Scale, 0.0000001f, 1000000.0f);
		}


		m_MousePosX = ImGui::GetIO().MousePos.x - viewportOffset.x;
		m_MousePosY = ImGui::GetIO().MousePos.y - viewportOffset.y;
		if(ImGui::IsKeyDown(ImGuiKey_Space) && io.MouseDown[ImGuiMouseButton_Right]) ImGui::OpenPopup((std::string("Viewport Settings##") + std::to_string(this->m_ID)).c_str());
	}
	ImVec2 wsize = ImGui::GetWindowSize();
	m_Width = wsize.x; m_Height = wsize.y;
	m_RendererViewport->m_AspectRatio = m_Width / (m_Height + 0.000000001f);
	ImGui::Image((ImTextureID)(uint64_t)m_RendererViewport->m_FrameBuffer->GetColorTexture(), wsize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::EndChild();
	this->ShowSettingPopUp();
	ImGui::End();
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
			"Shaded",
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