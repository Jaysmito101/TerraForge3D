#include "Generators/SimpleBiomeMixer.h"	
#include "Data/ApplicationState.h"

SimpleBiomeMixer::SimpleBiomeMixer(ApplicationState* appState)
{
	m_AppState = appState;
	m_RequireUpdation = true;
	// const auto shaderSource = ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "biome_mixer" PATH_SEPARATOR "simple_mixer.glsl", &s_TempBool);
	// m_Shader = std::make_shared<ComputeShader>(shaderSource);
	m_Shader = m_AppState->resourceManager->LoadComputeShader("generation/biome_mixer/simple_mixer");
}

SimpleBiomeMixer::~SimpleBiomeMixer()
{
}

void SimpleBiomeMixer::Update(GeneratorData* heightmapData, GeneratorData* m_SwapBuffer)
{
	const auto& biomeManagers = m_AppState->generationManager->GetBiomeManagers();
	auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;


	m_Shader->Bind();
	m_SwapBuffer->Bind(1);

	// clear the buffer
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->SetUniform1i("u_Mode", 0);
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	m_Shader->SetMemoryBarrier();

	std::unordered_map<std::string, SimpleBiomeMixerSettings> biomeSettingsMap;

	// mix the biomes
	m_Shader->SetUniform1i("u_Mode", 1);
	for (auto& biomeManager : biomeManagers)
	{
		auto settings = m_BiomeSettings.find(biomeManager->GetBiomeID());
		if (settings == m_BiomeSettings.end())  m_BiomeSettings[biomeManager->GetBiomeID()] = SimpleBiomeMixerSettings();
		auto& biomeSettings = m_BiomeSettings[biomeManager->GetBiomeID()];
		biomeSettingsMap[biomeManager->GetBiomeID()] = biomeSettings;		

		if (!biomeSettings.enabled || !biomeManager->IsEnabled()) continue;

		biomeManager->GetBiomeData()->Bind(0);
		m_Shader->SetUniform1f("u_Strength", biomeSettings.strength);
		m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
		m_Shader->SetMemoryBarrier();
	}

	// This is a horrible way to update the map, but it works for now
	// TODO: Make this better (PRs very welcome)
	m_BiomeSettings = biomeSettingsMap;

	m_SwapBuffer->CopyTo(heightmapData);

	m_RequireUpdation = false;

}

bool SimpleBiomeMixer::ShowSettings()
{
	auto& biomeManagers = m_AppState->generationManager->GetBiomeManagers();
	ImGui::Text("Biomes");

	for (auto& biomeManager : biomeManagers)
	{
		ImGui::PushID(biomeManager->GetBiomeID().c_str());

		if (ImGui::CollapsingHeader(biomeManager->GetBiomeName()))
		{
			auto settings = m_BiomeSettings.find(biomeManager->GetBiomeID());
			if (settings == m_BiomeSettings.end())  m_BiomeSettings[biomeManager->GetBiomeID()] = SimpleBiomeMixerSettings();
			auto& biomeSettings = m_BiomeSettings[biomeManager->GetBiomeID()];

			BIOME_UI_PROPERTY(ImGui::Checkbox("Enabled", &biomeSettings.enabled));
			BIOME_UI_PROPERTY(ImGui::SliderFloat("Strength", &biomeSettings.strength, 0.0f, 1.0f));
		}

		ImGui::PopID();
	}


	return m_RequireUpdation;
}
