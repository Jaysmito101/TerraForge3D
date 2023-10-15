#include "Generators/BiomeMixer.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

BiomeMixer::BiomeMixer(ApplicationState* appState)
{
	m_AppState = appState;
	m_Method = BiomeMixerMethod_Simple;
	m_RequireUpdation = true;

	m_SimpleBiomeMixer = std::make_shared<SimpleBiomeMixer>(appState);

}

BiomeMixer::~BiomeMixer()
{
}

void BiomeMixer::Update(GeneratorData* heightmapData, GeneratorData* m_SwapBuffer)
{
	switch (m_Method)
	{
	case BiomeMixerMethod_Simple:
		m_SimpleBiomeMixer->Update(heightmapData, m_SwapBuffer);
		break;
	case BiomeMixerMethod_AlphaBlend:
		break;
	case BiomeMixerMethod_Count:
		break;
	default:
		break;
	}
	m_RequireUpdation = false;
}

bool BiomeMixer::ShowSettings()
{
	ImGui::Text("Biome Mixer");

	static const char* s_Methods[] = {
		"Simple",
		"Alpha Blend"
	};

	int method = static_cast<int>(m_Method);
	
	BIOME_UI_PROPERTY(ShowComboBox("Method##BiomeMixerMethod", &method, s_Methods, 2));

	m_Method = static_cast<BiomeMixerMethod>(method);

	
	switch (m_Method)
	{
	case BiomeMixerMethod_Simple :
		BIOME_UI_PROPERTY(m_SimpleBiomeMixer->ShowSettings());
		break;
	case BiomeMixerMethod_AlphaBlend:
		ImGui::Text("TODO");
		break;
	}



	return m_RequireUpdation;
}
