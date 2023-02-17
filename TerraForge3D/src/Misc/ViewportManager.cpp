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
	if (m_AutoCalculateAspectRatio) m_RendererViewport->m_Camera.aspect = (m_Width / (m_Height + 0.000000001));
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
			m_RendererViewport->m_Camera.position[0] += -io.MouseDelta.x * 0.005f * glm::distance(glm::vec3(0.0f), glm::vec3(m_RendererViewport->m_Camera.position[0], m_RendererViewport->m_Camera.position[1], m_RendererViewport->m_Camera.position[2]));
			m_RendererViewport->m_Camera.position[1] += io.MouseDelta.y * 0.005f * glm::distance(glm::vec3(0.0f), glm::vec3(m_RendererViewport->m_Camera.position[0], m_RendererViewport->m_Camera.position[1], m_RendererViewport->m_Camera.position[2]));
		}
		if (io.MouseDown[ImGuiMouseButton_Middle] && !(ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)))
		{
			m_RendererViewport->m_Camera.rotation[0] += io.MouseDelta.x * 10.0f;
			m_RendererViewport->m_Camera.rotation[1] += io.MouseDelta.y * 10.0f;
		}
		if (fabs(io.MouseWheel) > 0.000001f)
		{
			m_RendererViewport->m_Camera.position[0] += m_AppState->constants.FRONT.x * 0.06f * io.MouseWheel;
			m_RendererViewport->m_Camera.position[1] += m_AppState->constants.FRONT.y * 0.06f * io.MouseWheel;
			m_RendererViewport->m_Camera.position[2] += m_AppState->constants.FRONT.z * 0.06f * io.MouseWheel;
		}


		m_MousePosX = ImGui::GetIO().MousePos.x - viewportOffset.x;
		m_MousePosY = ImGui::GetIO().MousePos.y - viewportOffset.y;
		if(ImGui::IsKeyDown(ImGuiKey_Space) && io.MouseDown[ImGuiMouseButton_Right]) ImGui::OpenPopup((std::string("Viewport Settings##") + std::to_string(this->m_ID)).c_str());
	}
	ImVec2 wsize = ImGui::GetWindowSize();
	m_Width = wsize.x; m_Height = wsize.y;
	ImGui::Image((ImTextureID)m_RendererViewport->m_FrameBuffer->GetColorTexture(), wsize, ImVec2(0, 1), ImVec2(1, 0));
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
		ImGui::NewLine();


		ImGui::EndPopup();
	}
}
