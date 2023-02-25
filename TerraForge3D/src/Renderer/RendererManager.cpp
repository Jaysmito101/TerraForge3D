#include "Renderer/RendererManager.h"

RendererManager::RendererManager(ApplicationState* appState)
{
	this->m_AppState = appState;
	this->m_ObjectRenderer = new ObjectRenderer(appState);
	this->m_HeightmapRenderer = new HeightmapRenderer(appState);
	this->m_ShadedRenderer = new ShadedRenderer(appState);
	this->m_TextureSlotRenderer = new TextureSlotRenderer(appState);
	this->m_WireframeRenderer = new WireframeRenderer(appState);

	this->m_RendererLights = new RendererLights(appState);
}

RendererManager::~RendererManager()
{
	delete m_ObjectRenderer;
	delete m_HeightmapRenderer;
	delete m_ShadedRenderer;
	delete m_TextureSlotRenderer;
	delete m_WireframeRenderer;

	delete m_RendererLights;
}

void RendererManager::Render(RendererViewport* viewport)
{
	switch (viewport->m_ViewportMode)
	{
	case RendererViewportMode_Object: m_ObjectRenderer->Render(viewport); break;
	case RendererViewportMode_Wireframe: m_WireframeRenderer->Render(viewport); break;
	case RendererViewportMode_Heightmap: m_HeightmapRenderer->Render(viewport); break;
	case RendererViewportMode_Shaded: m_ShadedRenderer->Render(viewport); break;
	case RendererViewportMode_TextureSlot: m_TextureSlotRenderer->Render(viewport); break;
	default: break;
	}
}

void RendererManager::ShowSettings()
{
	if (!this->m_IsWindowVisible) return;
	ImGui::Begin("Renderer Settings", &this->m_IsWindowVisible);
	ImGui::PushID("Renderer Settings");
	if (ImGui::CollapsingHeader("Core Settings"))
	{
		if (ImGui::BeginTabBar("Core Settings Type"))
		{
			if (ImGui::BeginTabItem("Object"))
			{
				ImGui::PushID("Core Settings Type->Object");
				m_ObjectRenderer->ShowSettings();
				ImGui::PopID();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Wireframe")) 
			{
				ImGui::PushID("Core Settings Type->Wireframe");
				m_WireframeRenderer->ShowSettings();
				ImGui::PopID();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Heightmap"))
			{
				ImGui::PushID("Core Settings Type->Heightmap");
				m_HeightmapRenderer->ShowSettings();
				ImGui::PopID();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Shaded"))
			{
				ImGui::PushID("Core Settings Type->Shaded");
				m_ShadedRenderer->ShowSettings();
				ImGui::PopID();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Texture Slot"))
			{
				ImGui::PushID("Core Settings Type->Texture Slot");
				m_TextureSlotRenderer->ShowSettings();
				ImGui::PopID();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::PopID();
	ImGui::Separator();
	ImGui::PushID("Items Settings");
	if (ImGui::CollapsingHeader("Items")) 
	{
		if (ImGui::BeginTabBar("Items Settings"))
		{
			if (ImGui::BeginTabItem("Terrain"))
			{
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Lights"))
			{
				m_RendererLights->ShowSettings();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sky"))
			{
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sea"))
			{
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Objects"))
			{
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::PopID();
	ImGui::Separator();
	ImGui::End();
}
