#include "Renderer/RendererManager.h"
#include "Data/ApplicationState.h"

RendererManager::RendererManager(ApplicationState* appState)
{
	m_AppState = appState;
	m_ObjectRenderer = new ObjectRenderer(appState);
	m_HeightmapRenderer = new HeightmapRenderer(appState);
	m_ShadedRenderer = new ShadedRenderer(appState);
	m_TextureSlotRenderer = new TextureSlotRenderer(appState);
	m_WireframeRenderer = new WireframeRenderer(appState);

	m_RendererLights = new RendererLights(appState);
	m_RendererSky = new RendererSky(appState);
}

RendererManager::~RendererManager()
{
	delete m_ObjectRenderer;
	delete m_HeightmapRenderer;
	delete m_ShadedRenderer;
	delete m_TextureSlotRenderer;
	delete m_WireframeRenderer;

	delete m_RendererLights;
	delete m_RendererSky;
}

void RendererManager::Render(RendererViewport* viewport)
{
	glBindFramebuffer(GL_FRAMEBUFFER, viewport->m_FrameBuffer->GetRendererID());
	glViewport(0, 0, viewport->m_FrameBuffer->GetWidth(), viewport->m_FrameBuffer->GetHeight());
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_RendererSky->Render(viewport);
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
				ImGui::PushID("Terrain");
				if (ImGui::Button("Generator Settings")) m_AppState->meshGenerator->windowStat = true;
				ImGui::PopID();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Lights"))
			{
				ImGui::PushID("Lights");
				m_RendererLights->ShowSettings();
				ImGui::PopID();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sky"))
			{
				ImGui::PushID("Sky");
				m_RendererSky->ShowSettings();
				ImGui::PopID();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sea"))
			{
				ImGui::PushID("Sea");
				ImGui::PopID();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Objects"))
			{
				ImGui::PushID("Objects");
				ImGui::PopID();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::PopID();
	ImGui::Separator();
	ImGui::End();
}
