#include "Generators/GenerationManager.h"
#include "Data/ApplicationState.h"
#include "Base/ComputeShader.h"
#include "Utils/Utils.h"
#include "Profiler.h"

static float a, b, c, d, e;
static bool ch = false;

GenerationManager::GenerationManager(ApplicationState* appState)
{
	m_AppState = appState;
	a = 1.0f;
	b = 1.0f;
	c = 1.0f;
	m_AppState->eventManager->Subscribe("TileResolutionChanged", BIND_EVENT_FN(OnTileResolutionChange));
	m_AppState->eventManager->Subscribe("ForceUpdate", BIND_EVENT_FN(UpdateInternal));
	m_HeightmapData = new GeneratorData();
	m_SwapBuffer = new GeneratorData();
	m_BiomeManagers.push_back(new BiomeManager(m_AppState));
}

GenerationManager::~GenerationManager()
{
	
}

void GenerationManager::Update()
{
	if (m_UpdationPaused) return;
	
	if (m_RequireUpdation)
	{
		UpdateInternal();
	}
}

bool GenerationManager::UpdateInternal(const std::string& params, void* paramsPtr)
{
	auto forceUpdate = params == "ForceUpdate";
	for (auto biome : m_BiomeManagers)
	{
		if (biome->IsUpdationRequired() || forceUpdate)
		{
			biome->Update(m_SwapBuffer);
		}
	}
	if (m_SelectedBiome != -1) m_BiomeManagers[m_SelectedBiome]->GetBiomeData()->CopyTo(m_HeightmapData);
	m_RequireUpdation = false;
	return false;
}


void GenerationManager::ShowSettings()
{
	ImGui::Begin("Generation Manager", &m_IsWindowVisible);
	ImGui::Checkbox("Updation Paused", &m_UpdationPaused);
	ImGui::Separator();
	ImGui::PushFont(GetUIFont("OpenSans-Bold"));
	ImGui::Text("Biomes");
	ImGui::PopFont();
	for (int i = 0; i < m_BiomeManagers.size(); i++)
	{
		auto biome = m_BiomeManagers[i];
		if (ImGui::Selectable(biome->GetBiomeName(), m_SelectedBiome == i))
		{
			m_SelectedBiome = i;
			m_AppState->eventManager->RaiseEvent("ForceUpdate", "ForceUpdate");
		}
	}

	ImGui::NewLine();
	if (m_SelectedBiome != -1)
	{
		ImGui::PushFont(GetUIFont("OpenSans-Bold"));
		ImGui::Text(m_BiomeManagers[m_SelectedBiome]->GetBiomeName());
		ImGui::PopFont();
		m_RequireUpdation = m_BiomeManagers[m_SelectedBiome]->ShowSettings() || m_RequireUpdation;
	}

	ImGui::End();
}

void GenerationManager::InvalidateWorkInProgress()
{
}

bool GenerationManager::IsWorking()
{
	return false;
}

bool GenerationManager::OnTileResolutionChange(const std::string params, void* paramsPtr)
{
	auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
	m_HeightmapData->Resize(size);
	m_SwapBuffer->Resize(size);
	m_RequireUpdation = true;
	for (auto biome : m_BiomeManagers)
	{
		biome->Resize();
	}
	return false;
}

