#pragma once

#include "Base/Base.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"

class ApplicationState;
class ComputeShader;

enum class CalculatedMaskType
{
	HeightRange,
	SlopeRange,
};

struct CalculatedMaskSettings
{
	CalculatedMaskType type = CalculatedMaskType::HeightRange;
	float minimum = 0.25f;
	float maximum = 0.75f;
	float softness = 0.05f;
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

	inline GeneratorTexture* GetTexture() const { return m_Texture.get(); }
	inline const CalculatedMaskSettings& GetSettings() const { return m_Settings; }

private:
	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<ComputeShader> m_Shader;
	std::shared_ptr<GeneratorTexture> m_Texture;
	CalculatedMaskSettings m_Settings;
	int m_Size = 256;
	bool m_Dirty = true;
};
