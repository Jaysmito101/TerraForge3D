#include "Generators/BiomeManager.h"
#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"

#include "Generators/BiomeBaseShapeGenerator.h"

bool BiomeManager::AddBaseShapeGenerator(const std::string& config)
{
	m_BaseShapeGenerators.push_back(std::make_shared<BiomeBaseShapeGenerator>(m_AppState));
	return m_BaseShapeGenerators.back()->LoadConfig(config);
}

bool BiomeManager::LoadUpResources()
{
	const std::string baseShapeGeneratorsDir = m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape";
	for (auto& directoryEntry : std::filesystem::recursive_directory_iterator(baseShapeGeneratorsDir))
	{
		if (!directoryEntry.is_directory() && directoryEntry.path().extension() == ".glsl")
		{
			const std::string config = ReadShaderSourceFile(directoryEntry.path().string(), &s_TempBool);
			if (!s_TempBool) continue;
			if (!AddBaseShapeGenerator(config)) Log("ERROR: Failed to load base shape generator " + directoryEntry.path().string());
		}
	}
	m_BaseNoiseGenerator = std::make_shared<BiomeBaseNoiseGenerator>(m_AppState);
	m_DEMBaseShapeGenerator = std::make_shared<DEMBaseShapeGenerator>(m_AppState);
	m_CustomBaseShape = std::make_shared<BiomeCustomBaseShape>(m_AppState);
	return true;
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
	LoadUpResources();
	// m_SelectedBaseShapeGeneratorMode = BiomeBaseShapeGeneratorMode_GlobalElevation;
	for (auto i = 0; i < static_cast<int32_t>(m_BaseShapeGenerators.size()); i++)
	{
		if (m_BaseShapeGenerators[i]->GetName() == "Classic")
		{
			m_SelectedBaseShapeGenerator = i;
			break;
		}
	}
}

BiomeManager::~BiomeManager()
{
}

void BiomeManager::Resize()
{
	auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
	m_Data->Resize(size);
	m_CustomBaseShape->Resize();
	m_RequireUpdation = true;
}

void BiomeManager::Update(GeneratorData* swapBuffer, GeneratorTexture* seedTexture)
{
	if (!m_IsEnabled) return;
	START_PROFILER();
	// m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->Update(m_Data, seedTexture);

	if (!m_CustomBaseShape->IsEnabled() || m_CustomBaseShape->RequiresBaseShapeUpdate())
	{
		if (m_SelectedBaseShapeGeneratorMode == BiomeBaseShapeGeneratorMode_Algorithm) 
		{
			m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->Update(m_Data.get(), seedTexture);
		}
		else if (m_SelectedBaseShapeGeneratorMode == BiomeBaseShapeGeneratorMode_GlobalElevation)
		{
			m_DEMBaseShapeGenerator->Update(m_Data.get(), seedTexture);
		}
	}

	if (m_CustomBaseShape->IsEnabled())
	{
		m_CustomBaseShape->Update(m_Data.get(), m_Data.get(), swapBuffer);
	}

	m_Data->CopyTo(swapBuffer);	 // temporary will later be 
	// optimized when filters are implemented

	m_BaseNoiseGenerator->Update(swapBuffer, m_Data.get(), seedTexture);


	END_PROFILER(m_CalculationTime);
	m_RequireUpdation = false;
}

bool BiomeManager::ShowCustomBaseShapeSettings()
{
	ImGui::PushID(m_BiomeID.data());
	BIOME_UI_PROPERTY(m_CustomBaseShape->ShowShettings());
	ImGui::PopID();
	return m_RequireUpdation;
}

bool BiomeManager::ShowBaseShapeSettings()
{
	ImGui::PushID(m_BiomeID.data());

	if (ImGui::BeginCombo("Base Shape Generator Mode", s_BaseShapeGeneratorModeNames[m_SelectedBaseShapeGeneratorMode].c_str()))
	{
		for (int i = 0; i < static_cast<int32_t>(s_BaseShapeGeneratorModeNames.size()); i++)
		{
			bool isSelected = m_SelectedBaseShapeGeneratorMode == i;
			if (ImGui::Selectable(s_BaseShapeGeneratorModeNames[i].c_str(), isSelected)) { m_SelectedBaseShapeGeneratorMode = static_cast<BiomeBaseShapeGeneratorMode>(i); m_RequireUpdation = true; }
			if (isSelected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (m_SelectedBaseShapeGeneratorMode == BiomeBaseShapeGeneratorMode_Algorithm)
	{
		if (ImGui::BeginCombo("Style", m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->GetName().c_str()))
		{
			for (int i = 0; i < static_cast<int32_t>(m_BaseShapeGenerators.size()); i++)
			{
				bool isSelected = m_SelectedBaseShapeGenerator == i;
				if (ImGui::Selectable(m_BaseShapeGenerators[i]->GetName().c_str(), isSelected)) { m_SelectedBaseShapeGenerator = i; m_RequireUpdation = true; }
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		BIOME_UI_PROPERTY(m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->ShowSettings());
	}
	else if (m_SelectedBaseShapeGeneratorMode == BiomeBaseShapeGeneratorMode_GlobalElevation)
	{
		BIOME_UI_PROPERTY(m_DEMBaseShapeGenerator->ShowSettings());
	}


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
	BIOME_UI_PROPERTY(m_BaseNoiseGenerator->ShowSettings());
	ImGui::PopID();
	return m_RequireUpdation;
}
