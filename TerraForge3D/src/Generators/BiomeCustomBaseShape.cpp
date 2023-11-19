 #include "Generators/BiomeCustomBaseShape.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"
#include "Profiler.h"

BiomeCustomBaseShape::BiomeCustomBaseShape(ApplicationState* appState)
{
	m_AppState = appState;

	m_WorkingDataBuffer = std::make_shared<GeneratorData>();
	m_SwapBuffer = std::make_shared<GeneratorData>();
	
	// this is just a preview texture, it is not used for anything else
	// so its ok for it to be low resolution
	m_PreviewTexture = std::make_shared<GeneratorTexture>(512, 512);

	// m_Shader = std::make_shared<ComputeShader>(appState->resourceManager->LoadShaderSource("generation/custom_base_shape/custom_base_shape"));
	m_Shader = m_AppState->resourceManager->LoadComputeShader("generation/custom_base_shape/custom_base_shape");


	m_RequireBaseShapeUpdate = true;
	m_RequireUpdation = true;
	m_Enabled = false;
}

BiomeCustomBaseShape::~BiomeCustomBaseShape()
{
}

bool BiomeCustomBaseShape::ShowShettings()
{
	if (ImGui::CollapsingHeader("Statistics"))
	{
		ImGui::Text("Time Taken: %f ms", m_CalculationTime);
	}

	bool enabledSwitch = (ImGui::Checkbox("Enabled", &m_Enabled));

	if (ImGui::Button("Reload Base Shape"))
	{
		m_RequireBaseShapeUpdate = true;
		m_RequireUpdation = true;
	}

	ImGui::SameLine();

	if (ImGui::Button("Flatten"))
	{
		m_WorkingDataBuffer->Bind(1);
		m_Shader->Bind();
		m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
		m_Shader->SetUniform1i("u_Mode", 0); // for transfer
		m_Shader->SetUniform1f("u_MixFactor", 0.0f);
		const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
		m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
		m_RequireUpdation = true;
	}

	if (ImGui::BeginTabBar("Core Settings Type"))
	{
		if (ImGui::BeginTabItem("Draw"))
		{
			ImGui::PushID("BiomeCustomBaseShape Edit Mode -> Draw");
			BIOME_UI_PROPERTY(ShowDrawEditor());
			ImGui::PopID();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	return m_RequireUpdation && m_Enabled || enabledSwitch;
}

void BiomeCustomBaseShape::Update(GeneratorData* sourceBuffer, GeneratorData* targetBuffer, GeneratorData* swapBuffer)
{
	START_PROFILER();

	if (m_RequireBaseShapeUpdate)
	{
		if (sourceBuffer)
		{
			sourceBuffer->CopyTo(m_WorkingDataBuffer.get());
		}
		else
		{
			Log("Source buffer found to be null when base shape load was requested!");
		}
		m_RequireBaseShapeUpdate = false;
	}


	m_WorkingDataBuffer->Bind(0);
	targetBuffer->Bind(1);
	m_Shader->Bind();
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->SetUniform1i("u_Mode", 0); // for transfer
	m_Shader->SetUniform1f("u_MixFactor", 1.0f);
	const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	m_Shader->SetMemoryBarrier();

	// m_WorkingDataBuffer->CopyTo(sourceBuffer);

	END_PROFILER(m_CalculationTime);

	m_RequireUpdation = false;
}

SerializerNode BiomeCustomBaseShape::Save()
{
	return CreateSerializerNode();
}

void BiomeCustomBaseShape::Load(SerializerNode node)
{
	
}

void BiomeCustomBaseShape::Resize()
{
	auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
	m_WorkingDataBuffer->Resize(size);
	m_SwapBuffer->Resize(size);
	m_RequireBaseShapeUpdate = true;
	m_RequireUpdation = true;
}

bool BiomeCustomBaseShape::ApplyDrawingShaders()
{
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		m_WorkingDataBuffer->Bind(1);
		m_Shader->Bind();
		m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
		if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || m_DrawSettings.m_BrushMode == 1)
		{
			m_Shader->SetUniform1i("u_Mode", 2); // for gaussian smooth brush
			m_WorkingDataBuffer->CopyTo(m_SwapBuffer.get());
			m_SwapBuffer->Bind(0);
		}
		else m_Shader->SetUniform1i("u_Mode", 1); // for basic brush
		m_Shader->SetUniform2f("u_BrushPosition", m_DrawSettings.m_BrushPositionX, m_DrawSettings.m_BrushPositionY);
		m_Shader->SetUniform4f("u_BrushSettings0", m_DrawSettings.m_BrushStrength, m_DrawSettings.m_BrushSize, m_DrawSettings.m_BrushFalloff, 0.0f);
		m_Shader->SetUniform1f("u_MixFactor", ImGui::IsKeyDown(ImGuiKey_LeftCtrl) ? -0.01f : 0.01f);
		const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
		m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
		m_RequireUpdation = true;
	}

	return m_RequireUpdation;
}

bool BiomeCustomBaseShape::ShowDrawEditor()
{
	static int s_PrevBrushMode = 0;

	if (!m_Enabled) return false;

	ViewportManager* activeViewport = nullptr;
	for (auto editor : m_AppState->viewportManagers)
	{
		if (editor->IsActive())
		{
			activeViewport = editor;
			break;
		}
	}

	if (activeViewport)
	{
		auto posOnTerrain = activeViewport->GetPositionOnTerrain();
		m_DrawSettings.m_BrushPositionX = posOnTerrain.x;
		m_DrawSettings.m_BrushPositionY = posOnTerrain.y;
		m_AppState->rendererManager->GetObjectRenderer()->SetCustomBaseShapeDrawSettings(&m_DrawSettings);

		if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
		{
			if (m_DrawSettings.m_BrushMode != 1) s_PrevBrushMode = m_DrawSettings.m_BrushMode;
			m_DrawSettings.m_BrushMode = 1;

			if (ImGui::IsKeyDown(ImGuiKey_R))
			{
				activeViewport->SetControlEnabled(false);
				m_DrawSettings.m_BrushSize += ImGui::GetIO().MouseWheel * 0.2f * (m_DrawSettings.m_BrushSize + 0.01f);
				m_DrawSettings.m_BrushSize = glm::clamp(m_DrawSettings.m_BrushSize, 0.0f, 2.0f); 
			}
			else if (ImGui::IsKeyDown(ImGuiKey_S))
			{
				activeViewport->SetControlEnabled(false);
				m_DrawSettings.m_BrushStrength += ImGui::GetIO().MouseWheel * 0.1f;
			}
			else if (ImGui::IsKeyDown(ImGuiKey_F))
			{
				activeViewport->SetControlEnabled(false);
				m_DrawSettings.m_BrushFalloff += ImGui::GetIO().MouseWheel * 0.05f;
				m_DrawSettings.m_BrushFalloff = glm::clamp(m_DrawSettings.m_BrushFalloff, 0.0f, 1.0f);
			}
		}
		else
		{
			m_DrawSettings.m_BrushMode = s_PrevBrushMode;
		}

		BIOME_UI_PROPERTY(ApplyDrawingShaders());		 
	}

	static const char* s_BrushModes[] = { "Basic", "Gaussian Smooth" };

	if(ShowComboBox("Brush Mode", &m_DrawSettings.m_BrushMode, s_BrushModes, IM_ARRAYSIZE(s_BrushModes))) s_PrevBrushMode = m_DrawSettings.m_BrushMode;
	ImGui::SliderFloat("Brush Size", &m_DrawSettings.m_BrushSize, 0.0f, 2.0f);
	ImGui::SliderFloat("Brush Fall Off", &m_DrawSettings.m_BrushFalloff, 0.0f, 1.0f);
	ImGui::DragFloat("Brush Strength", &m_DrawSettings.m_BrushStrength, 0.01f);

	return m_RequireUpdation;
}

