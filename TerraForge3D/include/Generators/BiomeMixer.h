#pragma once

#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/BiomeManager.h"
#include "Base/Base.h"

class ApplicationState;

class BiomeMixer 
{
public:
	BiomeMixer(ApplicationState* appState);
	~BiomeMixer();

	void Update(GeneratorData* heightmapData, GeneratorData* m_SwapBuffer);
	bool ShowSettings();

	inline bool RequireUpdation() { return m_RequireUpdation; }

private:
	bool m_RequireUpdation;
	ApplicationState* m_AppState;
};