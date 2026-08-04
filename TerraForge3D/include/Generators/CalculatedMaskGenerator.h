#pragma once

#include "Base/Base.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Misc/CustomInspector.h"
#include "Generators/NoiseAlgorithmCatalog.h"

class ApplicationState;
class ComputeShader;

enum class CalculatedMaskType
{
	HeightRange,
	SlopeRange,
	Aspect,
	Curvature,
	Roughness,
	Flatness,
	RidgeValley,
	Coastline,
	DistanceFromCoast,
	FlowWetness,
	AmbientOcclusion,
	Exposure,
	DistanceFromBorder,
	DistanceFromPointPath,
	HeightContour,
	ProceduralNoise,
	RadialGradient,
	Count,
};

struct CalculatedMaskSettings
{
	static constexpr int MaxPathPoints = 16;

	CalculatedMaskType type = CalculatedMaskType::HeightRange;
	int shaderMode = 0;
	float minimum = 0.25f;
	float maximum = 0.75f;
	float softness = 0.05f;
	float angle = 0.0f;
	float angleWidth = 45.0f;
	float scale = 4.0f;
	float seed = 42.0f;
	int noiseAlgorithm = 0;
	int noiseOctaves = 5;
	float noiseLacunarity = 2.0f;
	float noisePersistence = 0.5f;
	float noiseWarp = 0.0f;
	float noiseJitter = 0.75f;
	float sampleRadius = 3.0f;
	float curvatureScale = 32.0f;
	float cavityScale = 48.0f;
	float seaLevel = 0.0f;
	glm::vec2 center = glm::vec2(0.5f);
	glm::vec2 pathEnd = glm::vec2(0.75f, 0.5f);
	std::array<glm::vec2, MaxPathPoints> pathPoints{};
	int pathPointCount = 2;
	bool usePath = false;
	bool selectValleys = false;
};

class CalculatedMaskGenerator
{
public:
	CalculatedMaskGenerator(ApplicationState* state);
	~CalculatedMaskGenerator();

	void Resize(int size);
	void Invalidate();
	bool ShowSettings();
	bool Update(GeneratorData* sourceData);
	SerializerNode Save() const;
	void Load(SerializerNode data);

	inline GeneratorTexture* GetTexture() const { return m_Texture.get(); }
	inline const CalculatedMaskSettings& GetSettings() const { return m_Settings; }

private:
	bool LoadMetadata();
	bool LoadInspectorForType(int typeIndex);
	void SyncSettingsFromInspector();
	int GetSelectedTypeIndex() const;
	int FindTypeIndexByID(const std::string& id) const;

	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<ComputeShader> m_Shader;
	std::shared_ptr<GeneratorTexture> m_Texture;
	std::shared_ptr<CustomInspector> m_Inspector;
	NoiseAlgorithmCatalog m_NoiseAlgorithms;
	nlohmann::json m_Metadata;
	CalculatedMaskSettings m_Settings;
	int m_Size = 256;
	bool m_Dirty = true;
	bool m_MetadataLoaded = false;
};
