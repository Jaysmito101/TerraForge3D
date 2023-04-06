#include "Generators/BiomeManager.h"
#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"

#include "Generators/BiomeBaseShapeGenerator.h"

std::vector<std::shared_ptr<BiomeBaseShapeGenerator>> BiomeManager::s_BaseShapeGenerators;

bool BiomeManager::AddBaseShapeGenerator(const std::string& config, ApplicationState* appState)
{
	s_BaseShapeGenerators.push_back(std::make_shared<BiomeBaseShapeGenerator>(appState));
	return s_BaseShapeGenerators.back()->LoadConfig(config);
}

bool BiomeManager::BiomeManager::LoadBaseShapeGenerators(ApplicationState* appState)
{
	static bool s_Success = false;
	if (!AddBaseShapeGenerator(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "flat.glsl", &s_Success), appState)) return false;
	if (!AddBaseShapeGenerator(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "classic.glsl", &s_Success), appState)) return false;
	if (!AddBaseShapeGenerator(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "cliff.glsl", &s_Success), appState)) return false;
	if (!AddBaseShapeGenerator(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "cracks.glsl", &s_Success), appState)) return false;
	if (!AddBaseShapeGenerator(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "crater.glsl", &s_Success), appState)) return false;
	if (!AddBaseShapeGenerator(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "dunes.glsl", &s_Success), appState)) return false;
	return true;
}

void BiomeManager::FreeBaseShapeGenerators()
{
	s_BaseShapeGenerators.clear();
}

BiomeManager::BiomeManager(ApplicationState* appState)
{
	m_AppState = appState;
	m_BiomeID = GenerateId(8);
	m_Color = ImVec4((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1.0f);
	bool success = false;
	m_Data = std::make_shared<GeneratorData>();
	static int s_BiomeID = 1;
	sprintf(m_BiomeName, "Biome %d", s_BiomeID++);
}

BiomeManager::~BiomeManager()
{
}

void BiomeManager::Resize()
{
	auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
	m_Data->Resize(size);
	m_RequireUpdation = true;
}

void BiomeManager::Update(GeneratorData* swapBuffer, GeneratorTexture* seedTexture)
{
	if (!m_IsEnabled) return;
	START_PROFILER();
	// m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->Update(m_Data, seedTexture);
	s_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->Update(m_Data.get(), seedTexture);
	END_PROFILER(m_CalculationTime);
	m_RequireUpdation = false;
}

bool BiomeManager::ShowCustomBaseShapeSettings()
{
	ImGui::PushID(m_BiomeID.data());
	ImGui::Text("Yet to be implemented!");
	ImGui::PopID();
	return m_RequireUpdation;
}

bool BiomeManager::ShowBaseShapeSettings()
{
	ImGui::PushID(m_BiomeID.data());
	if (ImGui::BeginCombo("Style", s_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->GetName().c_str()))
	{
		for (int i = 0; i < static_cast<int32_t>(s_BaseShapeGenerators.size()); i++)
		{
			bool is_selected = m_SelectedBaseShapeGenerator == i;
			if (ImGui::Selectable(s_BaseShapeGenerators[i]->GetName().c_str(), is_selected)) { m_SelectedBaseShapeGenerator = i; m_RequireUpdation = true; }
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	BIOME_UI_PROPERTY(s_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->ShowSettings());
	ImGui::PopID();
	return m_RequireUpdation;
}

bool BiomeManager::ShowGeneralSettings()
{
	ImGui::PushID(m_BiomeID.data());
	ImGui::InputText("Biome Name", m_BiomeName, sizeof(m_BiomeName));
	BIOME_UI_PROPERTY(ImGui::Checkbox("Enabled", &m_IsEnabled));
	ImGui::ColorEdit3("Biome Color", reinterpret_cast<float*>(&m_Color));
	if (ImGui::CollapsingHeader("Statistics"))
	{
		ImGui::Text("Time Taken: %f", m_CalculationTime);
	}
	ImGui::PopID();
	return m_RequireUpdation;
}

bool BiomeManager::ShowBaseNoiseSettings()
{
	ImGui::PushID(m_BiomeID.data());
	ImGui::Text("Yet to be implemented!");
	ImGui::PopID();
	return m_RequireUpdation;
}
