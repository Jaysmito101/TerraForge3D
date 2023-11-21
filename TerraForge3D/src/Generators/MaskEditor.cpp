#include "Generators/MaskEditor.h"
#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"

MaskEditor* MaskEditor::s_CurrentlyEditingMaskEditor = nullptr;

MaskEditor::MaskEditor(ApplicationState* state, glm::vec3 vizColor)
{
	m_AppState = state;
	m_VizColor = vizColor;


	m_Texture = std::make_shared<GeneratorTexture>(m_Size, m_Size);
	m_Shader = m_AppState->resourceManager->LoadComputeShader("generation/utils/mask_editor");
	m_DrawSettings.m_MaskTexture = m_Texture->GetRendererID();

}

MaskEditor::~MaskEditor()
{
}

void MaskEditor::Resize(int size)
{
	m_Texture->Resize(size, size);
}


bool MaskEditor::ApplyDrawingShaders()
{
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		m_Texture->BindForCompute(0);
		m_Shader->Bind();
		m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
		m_Shader->SetUniform1i("u_Mode", m_DrawSettings.m_BrushMode);
		m_Shader->SetUniform2f("u_BrushPosition", m_DrawSettings.m_BrushPositionX, m_DrawSettings.m_BrushPositionY);
		m_Shader->SetUniform4f("u_BrushSettings0", m_DrawSettings.m_BrushStrength, m_DrawSettings.m_BrushSize, m_DrawSettings.m_BrushFalloff, 0.0f);
		const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
		m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
		m_RequireUpdation = true;
	}

	return m_RequireUpdation;
}

bool MaskEditor::ShowSettings()
{
	m_RequireUpdation = false;

	ImGui::Separator();
	ImGui::Text("Mask Editor");

	ImGui::Image(m_Texture->GetTextureID(), ImVec2(256, 256));

	if (!m_IsEditing && ImGui::Button("Edit##MaskEditorEdit"))
	{

		if (s_CurrentlyEditingMaskEditor != nullptr)
		{
			s_CurrentlyEditingMaskEditor->m_IsEditing = false;
		}

		m_IsEditing = true;
		s_CurrentlyEditingMaskEditor = this;

	}

	if (m_IsEditing)
	{
		if (ImGui::Button("Stop Editing##MaskEditorStopEditing"))
		{
			m_IsEditing = false;
			s_CurrentlyEditingMaskEditor = nullptr;
		}

		static const char* s_BrushModes[] = { "Add", "Remove" };
		static int s_PrevBrushMode = 0;


		if (ShowComboBox("Brush Mode", &m_DrawSettings.m_BrushMode, s_BrushModes, IM_ARRAYSIZE(s_BrushModes))) s_PrevBrushMode = m_DrawSettings.m_BrushMode;
		ImGui::SliderFloat("Brush Size", &m_DrawSettings.m_BrushSize, 0.0f, 2.0f);
		ImGui::SliderFloat("Brush Fall Off", &m_DrawSettings.m_BrushFalloff, 0.0f, 1.0f);
		ImGui::DragFloat("Brush Strength", &m_DrawSettings.m_BrushStrength, 0.01f);


		ViewportManager* activeViewport = nullptr;
		for (auto editor : m_AppState->viewportManagers)
		{
			if (editor->IsActive())
			{
				activeViewport = editor;
				break;
			}
		}

		m_DrawSettings.m_MaskColor = m_VizColor;
		m_DrawSettings.m_BrushPositionX = m_DrawSettings.m_BrushPositionY = -1000.0f;

		if (activeViewport)
		{

			auto posOnTerrain = activeViewport->GetPositionOnTerrain();
			m_DrawSettings.m_BrushPositionX = posOnTerrain.x;
			m_DrawSettings.m_BrushPositionY = posOnTerrain.y;



			if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
			{
				if (ImGui::IsKeyDown(ImGuiKey_R))
				{
					activeViewport->SetControlEnabled(false);
					m_DrawSettings.m_BrushSize += ImGui::GetIO().MouseWheel * 0.2f * (m_DrawSettings.m_BrushSize + 0.01f);
					m_DrawSettings.m_BrushSize = glm::clamp(m_DrawSettings.m_BrushSize, 0.0f, 2.0f);
				}
				else if (ImGui::IsKeyDown(ImGuiKey_S))
				{
					activeViewport->SetControlEnabled(false);
					m_DrawSettings.m_BrushStrength += ImGui::GetIO().MouseWheel * 0.01f;
				}
				else if (ImGui::IsKeyDown(ImGuiKey_F))
				{
					activeViewport->SetControlEnabled(false);
					m_DrawSettings.m_BrushFalloff += ImGui::GetIO().MouseWheel * 0.05f;
					m_DrawSettings.m_BrushFalloff = glm::clamp(m_DrawSettings.m_BrushFalloff, 0.0f, 1.0f);
				}
			}
			else if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
			{
				if (m_DrawSettings.m_BrushMode != 1) s_PrevBrushMode = m_DrawSettings.m_BrushMode;
				m_DrawSettings.m_BrushMode = 1;
			}
			else
			{
				m_DrawSettings.m_BrushMode = s_PrevBrushMode;
			}
			
			BIOME_UI_PROPERTY(ApplyDrawingShaders());
		}

		m_AppState->rendererManager->GetObjectRenderer()->SetCustomBaseShapeDrawSettings(&m_DrawSettings);
	}

	ImGui::Separator();
	return m_RequireUpdation;
}
