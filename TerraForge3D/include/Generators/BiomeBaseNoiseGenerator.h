#pragma once

#include <json.hpp>
#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Exporters/Serializer.h"
#include "Misc/CustomInspector.h"

#define BASE_SHAPE_UI_PROPERTY(x) m_RequireUpdation = x || m_RequireUpdation

#define BIOME_BASE_NOISE_OCTAVE_COUNT 10

class ApplicationState;

class BiomeBaseNoiseGenerator
{
public:
	BiomeBaseNoiseGenerator(ApplicationState* appState);
	~BiomeBaseNoiseGenerator();

	bool ShowSettings();
	void Update(GeneratorData* sourceBuffer, GeneratorData* targetBuffer, GeneratorTexture* seedTexture);

	void Load(SerializerNode data);
	SerializerNode Save();

	inline bool RequireUpdation() const { return m_RequireUpdation; }

private:
	ApplicationState* m_AppState = nullptr;
	float m_CalculationTime = 0.0f;

	bool m_RequireUpdation = true;
	std::shared_ptr<ComputeShader> m_Shader;
	std::shared_ptr<CustomInspector> m_Inspector;
	// this is not stored inside the inspector because we dont need any 
	// fancy ui for it and this will be more efficient
	float m_NoiseOctaveStrengths[BIOME_BASE_NOISE_OCTAVE_COUNT];
};