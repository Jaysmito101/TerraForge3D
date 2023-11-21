#pragma once

#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/BiomeManager.h"
#include "Base/Base.h"

class ApplicationState;

struct SimpleBiomeMixerSettings
{
	bool enabled = true;
	float strength = 1.0f;
	bool useBiomeMask = false;
};

class SimpleBiomeMixer
{
public:
	SimpleBiomeMixer(ApplicationState* state);
	~SimpleBiomeMixer();

	void Update(GeneratorData* heightmapData, GeneratorData* m_SwapBuffer);
	bool ShowSettings();

private:
	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<ComputeShader> m_Shader;
	std::unordered_map<std::string, SimpleBiomeMixerSettings> m_BiomeSettings;
	bool m_RequireUpdation = true;
};
