#include "Generators/BiomeMixer.h"
#include "Data/ApplicationState.h"

BiomeMixer::BiomeMixer(ApplicationState* appState)
{
	m_AppState = appState;
}

BiomeMixer::~BiomeMixer()
{
}

void BiomeMixer::Update(GeneratorData* heightmapData, GeneratorData* m_SwapBuffer)
{
	const auto& biomeManagers = m_AppState->generationManager->GetBiomeManagers();

	if (biomeManagers.size() > 1)
	{
		biomeManagers[1]->GetBiomeData()->CopyTo(heightmapData);
	}
	else {
		// TODO: Remove this hack
		auto hmapCPU = heightmapData->GetCPUCopy();
		memset(hmapCPU, 0, heightmapData->GetSize());
		heightmapData->SetData(hmapCPU, 0, heightmapData->GetSize());
		delete[] hmapCPU;
	}
	m_RequireUpdation = false;
}

bool BiomeMixer::ShowSettings()
{
	ImGui::Text("Biome Mixer");

	return m_RequireUpdation;
}
