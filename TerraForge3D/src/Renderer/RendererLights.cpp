#include "Renderer/RendererLights.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

namespace tf3d::renderer
{

    RendererLights::RendererLights(ApplicationState *appState)
    {
        m_AppState = appState;
        std::sprintf(m_Sun.name, "Sun");
    }

    RendererLights::~RendererLights()
    {
    }

    void RendererLights::ShowSettings()
    {
        ImGui::Text("Light Settings");
        ImGui::Checkbox("Use Sky Light", &m_UseSkyLight);
        ImGui::SliderFloat("Sky Light Intensity", &m_SkyLightIntensity, 0.0f, 1.0f);
        if (ImGui::BeginPopupContextItem()) {
            ImGui::Text("Enabling this will mean using environment lighting from the loaded skybox.");
            ImGui::EndPopup();
        }
        ImGui::Separator();
        ImGui::TextUnformatted(m_Sun.name);
        if (ImGui::BeginPopupContextItem()) {
            ImGui::InputText("Name", m_Sun.name, sizeof(m_Sun.name));
            ImGui::EndPopup();
        }
        ImGui::DragFloat3("Direction", glm::value_ptr(m_Sun.direction), 0.01f);
        ImGui::ColorEdit3("Color", glm::value_ptr(m_Sun.color));
        ImGui::DragFloat("Intensity", &m_Sun.intensity, 0.01f, 0.0f, 100.0f);
    }

} // namespace tf3d::renderer