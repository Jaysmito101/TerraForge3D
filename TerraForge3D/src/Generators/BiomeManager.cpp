#include "Generators/BiomeManager.h"
#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"

//#include "Generators/BiomeBaseShape/BiomeBaseShape_Flat.h"
//#include "Generators/BiomeBaseShape/BiomeBaseShape_Classic.h"
//#include "Generators/BiomeBaseShape/BiomeBaseShape_Cracks.h"
//#include "Generators/BiomeBaseShape/BiomeBaseShape_Cliff.h"
#include "Generators/BiomeBaseShapeGenerator.h"

BiomeManager::BiomeManager(ApplicationState* appState)
{
	m_AppState = appState;
	m_BiomeID = GenerateId(8);
	m_Color = ImVec4((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1.0f);
	bool success = false;
	m_Data = new GeneratorData();
	static int s_BiomeID = 1;
	sprintf(m_BiomeName, "Biome %d", s_BiomeID++);
	//m_BaseShapeGenerators.push_back(new BiomeBaseShape_Flat(m_AppState));
	//m_BaseShapeGenerators.push_back(new BiomeBaseShape_Classic(m_AppState));
	//m_BaseShapeGenerators.push_back(new BiomeBaseShape_Cracks(m_AppState));
	//m_BaseShapeGenerators.push_back(new BiomeBaseShape_Cliff(m_AppState));
	
	m_BaseShapeGenerator = std::make_shared<BiomeBaseShapeGenerator>(m_AppState);
	m_BaseShapeGenerator->LoadConfig(ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "classic2.glsl", &s_TempBool));
}

BiomeManager::~BiomeManager()
{
	delete m_Data;
// 	for (auto& generator : m_BaseShapeGenerators) delete generator;

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
	m_BaseShapeGenerator->Update(m_Data, seedTexture);
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
	/*
	if (ImGui::BeginCombo("Style", m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->GetName().c_str()))
	{
		for (int i = 0; i < (int)m_BaseShapeGenerators.size(); i++)
		{
			bool is_selected = m_SelectedBaseShapeGenerator == i;
			if (ImGui::Selectable(m_BaseShapeGenerators[i]->GetName().c_str(), is_selected)) { m_SelectedBaseShapeGenerator = i; m_RequireUpdation = true; }
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	*/

	// BIOME_UI_PROPERTY(m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->ShowSettings());
	BIOME_UI_PROPERTY(m_BaseShapeGenerator->ShowSettings());
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
