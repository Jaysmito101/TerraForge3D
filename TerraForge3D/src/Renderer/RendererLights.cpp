#include "Renderer/RendererLights.h"
#include "Utils/Utils.h"

RendererLights::RendererLights(ApplicationState* appState)
{
	m_AppState = appState;
	RendererLightData defaultLight;
	defaultLight.type = RendererLightType_Directional;
	defaultLight.position = glm::vec3(-1.0f, -1.0f, -1.0f);
	std::sprintf(defaultLight.name, "Sun");
	defaultLight.intensity = 0.5f;
	m_RendererLights.push_back(defaultLight);
}

RendererLights::~RendererLights()
{
}

void RendererLights::ShowSettings()
{
	ImGui::Text("Light Settings");
	ImGui::Checkbox("Use Sky Light", &m_UseSkyLight);
	if (ImGui::BeginPopupContextItem())
	{
		ImGui::Text("Enabling this will mean using environment lighting from the loaded skybox.");
		ImGui::EndPopup();
	}
	ImGui::Separator();
	for (int i = 0; i < m_RendererLights.size(); i++)
	{
		ImGui::PushID(i);
		ImGui::Selectable(m_RendererLights[i].name);
		if (ImGui::BeginPopupContextItem())
		{
			ImGui::InputText("Name", m_RendererLights[i].name, sizeof(m_RendererLights[i].name));
			ImGui::EndPopup();
		}
		static const char* s_RenderLightTypeNames[] = { "Directional", "Point" };
		SHOW_COMBO_BOX("Type", m_RendererLights[i].type, s_RenderLightTypeNames, IM_ARRAYSIZE(s_RenderLightTypeNames));
		ImGui::DragFloat3(m_RendererLights[i].type == RendererLightType_Directional ? "Direction" : "Position", glm::value_ptr(m_RendererLights[i].position), 0.01f);
		ImGui::ColorEdit3("Color", glm::value_ptr(m_RendererLights[i].color));
		ImGui::DragFloat("Intensity", &m_RendererLights[i].intensity, 0.01f);
		if (ImGui::Button("Duplicate")) m_RendererLights.push_back(m_RendererLights[i]);
		ImGui::SameLine();
		if (ImGui::Button("Delete"))
		{
			m_RendererLights.erase(m_RendererLights.begin() + i);
			ImGui::PopID(); break;
		}
		ImGui::PopID();
		ImGui::Separator();
	}
	if (ImGui::Button("Add Light")) m_RendererLights.emplace_back();
}
