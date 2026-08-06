#include "Misc/ViewportManager.h"
#include "Base/Base.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

#include <string>

namespace tf3d::misc
{

    ViewportManager::ViewportManager(ApplicationState *appState)
    {
        static uint32_t s_ViewportID = 1;
        this->m_ID                   = s_ViewportID++;
        this->m_IsVisible            = false;
        if (this->m_ID == 1)
            this->m_IsVisible = true; // Show the first viewport by default
        this->m_AppState         = appState;
        this->m_RendererViewport = new renderer::RendererViewport();
    }

    ViewportManager::~ViewportManager()
    {
        delete this->m_RendererViewport;
    }

    void ViewportManager::Update()
    {
        TF3D_PROFILE_SCOPE(std::string("viewport/") + std::to_string(m_ID) + "/update");
        if (m_AutoCalculateAspectRatio) {
            m_RendererViewport->GetCamera().SetAspectRatio(m_Width / (m_Height + 0.000000001f));
        }

        if (m_IsVisible) {
            const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;
            const float scaleX            = std::max(framebufferScale.x, 1.0f);
            const float scaleY            = std::max(framebufferScale.y, 1.0f);
            constexpr float renderScale   = 1.5f;
            const uint32_t renderWidth    = static_cast<uint32_t>(std::max(m_Width * scaleX * renderScale, 1.0f));
            const uint32_t renderHeight   = static_cast<uint32_t>(std::max(m_Height * scaleY * renderScale, 1.0f));
            {
                TF3D_PROFILE_SCOPE(std::string("viewport/") + std::to_string(m_ID) + "/resize");
                m_RendererViewport->ResizeTo(renderWidth, renderHeight);
            }
            this->m_AppState->rendererManager->Render(this->m_RendererViewport);
        } else {
            TF3D_PROFILE_SCOPE(std::string("viewport/") + std::to_string(m_ID) + "/hidden");
        }
        m_IsActive &= m_IsVisible;
    }

    void ViewportManager::Show()
    {
        static char s_TempBuffer[1024];
        std::sprintf(s_TempBuffer, "Viewport %d", this->m_ID);
        if (!this->m_IsVisible)
            return;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin(s_TempBuffer, &m_IsVisible);
        ImGui::PopStyleVar();
        // auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
        // auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        auto viewportOffset = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin();
        ImGui::BeginChild("MainRender");
        m_RendererViewport->SetHovered(ImGui::IsWindowHovered());
        if (m_RendererViewport->IsHovered()) {
            ImGuiIO io = ImGui::GetIO();

            if (m_IsControlEnabled) {
                const bool usesCamera = m_RendererViewport->GetMode() == renderer::RendererViewportMode_Object ||
                                        m_RendererViewport->GetMode() == renderer::RendererViewportMode_Wireframe;
                if (usesCamera && ImGui::IsKeyPressed(ImGuiKey_Space))
                    m_RendererViewport->GetCamera().Reset();
                if (io.MouseDown[ImGuiMouseButton_Middle] && (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift))) {
                    if (usesCamera) {
                        m_RendererViewport->GetCamera().Pan(io.MouseDelta.x * m_MovementSpeed, io.MouseDelta.y * m_MovementSpeed, m_Height);
                    } else {
                        m_RendererViewport->GetOffsetX() -= io.MouseDelta.x * m_MovementSpeed * 0.001f;
                        m_RendererViewport->GetOffsetY() += io.MouseDelta.y * m_MovementSpeed * 0.001f;
                    }
                }
                if (io.MouseDown[ImGuiMouseButton_Middle] && !(ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift))) {
                    if (usesCamera) {
                        m_RendererViewport->GetCamera().Orbit(io.MouseDelta.x * m_RotationSpeed, -io.MouseDelta.y * m_RotationSpeed);
                    } else {
                        m_RendererViewport->GetOffsetX() -= io.MouseDelta.x * m_MovementSpeed * 0.001f;
                        m_RendererViewport->GetOffsetY() -= io.MouseDelta.y * m_MovementSpeed * 0.001f;
                    }
                }
                if (fabs(io.MouseWheel) > 0.000001f) {
                    if (usesCamera) {
                        m_RendererViewport->GetCamera().Zoom(io.MouseWheel * m_ZoomSpeed);
                    } else {
                        m_RendererViewport->GetScale() += m_ZoomSpeed * 0.06f * io.MouseWheel;
                        m_RendererViewport->GetScale() = glm::clamp(m_RendererViewport->GetScale(), 0.0000001f, 1000000.0f);
                    }
                }
                if (usesCamera && ImGui::IsKeyPressed(ImGuiKey_F))
                    m_RendererViewport->GetCamera().Reset();

                if (ImGui::IsKeyDown(ImGuiKey_Space) && io.MouseDown[ImGuiMouseButton_Right])
                    ImGui::OpenPopup((std::string("Viewport Settings##") + std::to_string(this->m_ID)).c_str());
            }

            auto &mousePosition = m_RendererViewport->GetMousePosition();
            mousePosition[0] = m_MousePosX = (ImGui::GetIO().MousePos.x - viewportOffset.x) / m_Width;
            mousePosition[1] = m_MousePosY = (ImGui::GetIO().MousePos.y - viewportOffset.y) / m_Height;
            mousePosition[1]               = 1.0f - mousePosition[1];

        } else {
            auto &mousePosition = m_RendererViewport->GetMousePosition();
            mousePosition[0] = m_MousePosX = -1.0f;
            mousePosition[1] = m_MousePosY = -1.0f;
        }

        ImVec2 imageSize = ImGui::GetWindowSize();
        m_Width          = imageSize.x;
        m_Height         = imageSize.y;
        m_RendererViewport->SetAspectRatio(m_Width / (m_Height + 0.000000001f));
        ImGui::Image((ImTextureID)(uint64_t)m_RendererViewport->GetFrameBuffer()->GetColorTexture(), imageSize, ImVec2(0, 1), ImVec2(1, 0));
        m_IsActive = ImGui::IsItemHovered();
        ImGui::EndChild();
        this->ShowSettingPopUp();
        ImGui::End();

        m_IsControlEnabled = true; // temp
    }

    void ViewportManager::ShowSettingPopUp()
    {
        if (ImGui::BeginPopupContextItem((std::string("Viewport Settings##") + std::to_string(this->m_ID)).c_str())) {
            static const char *viewportModesText[] =
                {
                    "Object",
                    "Wireframe",
                    "Heightmap",
                    "TextureSlot"};

            int viewportMode = static_cast<int>(m_RendererViewport->GetMode());
            ShowComboBox("Viewport Mode", &viewportMode, viewportModesText, IM_ARRAYSIZE(viewportModesText));
            m_RendererViewport->SetMode(static_cast<renderer::RendererViewportMode>(viewportMode));
            ImGui::NewLine();

            if (m_RendererViewport->GetMode() != renderer::RendererViewportMode_Heightmap && m_RendererViewport->GetMode() != renderer::RendererViewportMode_TextureSlot) {
                ImGui::Text("Camera Settings");
                ImGui::Separator();
                m_RendererViewport->GetCamera().ShowSettings();
                ImGui::Checkbox("Auto Calculate Aspect Ratio", &m_AutoCalculateAspectRatio);
            } else {
                ImGui::DragFloat("Offset X", &m_RendererViewport->GetOffsetX(), 0.01f);
                ImGui::DragFloat("Offset Y", &m_RendererViewport->GetOffsetY(), 0.01f);
                ImGui::DragFloat("Scale", &m_RendererViewport->GetScale(), 0.01f);
            }

            if (m_RendererViewport->GetMode() == renderer::RendererViewportMode_TextureSlot) {
                auto &textureSlotDetailedMode = m_RendererViewport->GetTextureSlotDetailedMode();
                auto &textureSlot             = m_RendererViewport->GetTextureSlot();
                auto &textureSlotDetailed     = m_RendererViewport->GetTextureSlotDetailed();
                ImGui::Checkbox("Texture Slot Detailed Mode", &textureSlotDetailedMode);
                if (textureSlotDetailedMode) {
                    static const char *s_TextureSlotChannels[] = {"R", "G", "B", "A"};
                    for (int i = 0; i < 4; i++) {
                        ImGui::PushID(i);
                        ImGui::Text("Viewport Channel %s :", s_TextureSlotChannels[i]);
                        if (ImGui::DragInt("Texture Slot", &textureSlotDetailed[i].first, 0.1f, 0, 5))
                            textureSlotDetailed[i].first = glm::clamp(textureSlotDetailed[i].first, 0, 5);
                        ShowTextureSlotDetailsPopup();
                        if (ImGui::DragInt("Texture Slot Channel", &textureSlotDetailed[i].second, 0.1f, 0, 3))
                            textureSlotDetailed[i].first = glm::clamp(textureSlotDetailed[i].first, 0, 3);
                        ImGui::PopID();
                    }

                } else {
                    if (ImGui::DragInt("Texture Slot", &textureSlot, 0.1f, 0, 5))
                        textureSlot = glm::clamp(textureSlot, 0, 5);
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
        if (ImGui::BeginPopupContextItem()) {
            ImGui::Text("0 : [Heightmap, Custom, Custom, Custom]");
            ImGui::Text("1 : [Custom, Custom, Custom, Custom]");
            ImGui::Text("2 : [Custom, Custom, Custom, Custom]");
            ImGui::Text("3 : Albedo");
            ImGui::Text("4 : Normal");
            ImGui::Text("%s", "5 : [AO, Roughness, Metallic, Unused]");
            ImGui::EndPopup();
        }
    }

} // namespace tf3d::misc
