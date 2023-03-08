#include "Generators/BiomeManager.h"
#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"

BiomeManager::BiomeManager(ApplicationState* appState)
{
	m_AppState = appState;
	m_BiomeID = GenerateId(8);
	bool success = false;
	m_BiomeData = new GeneratorData();
	m_Shader = new ComputeShader(ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "dummy.glsl", &success));
	sprintf(m_BiomeName, "Default");
}

BiomeManager::~BiomeManager()
{
	delete m_BiomeData;
	delete m_Shader;
}

void BiomeManager::Resize()
{
	auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
	m_BiomeData->Resize(size);
	m_RequireUpdation = true;
}

static float a = 1.0f, b = 1.0f, c = 1.0f;

void BiomeManager::Update(GeneratorData* swapBuffer)
{
	START_PROFILER();
	m_BiomeData->Bind(0);
	m_Shader->Bind();
	m_Shader->SetUniform1f("a", a);
	m_Shader->SetUniform1f("b", b);
	m_Shader->SetUniform1f("c", c);
	m_Shader->SetUniform1i("r", m_AppState->mainMap.tileResolution);
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / 16, m_AppState->mainMap.tileResolution / 16, 1);
	END_PROFILER(m_CalculationTime);
	m_RequireUpdation = false;
}

bool BiomeManager::ShowSettings()
{
	ImGui::PushID(m_BiomeID.data());
	BIOME_UI_PROPERTY(ImGui::Checkbox("Enabled", &m_IsEnabled));
	BIOME_UI_PROPERTY(ImGui::DragFloat("a", &a, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("b", &b, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("c", &c, 0.01f));
	if (ImGui::CollapsingHeader("Statistics"))
	{
		ImGui::Text("Time Taken: %f", m_CalculationTime);
	}
	ImGui::PopID();
	return m_RequireUpdation;
}
