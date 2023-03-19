#pragma once
#include "BiomeBaseShapeGenerator.h"

class BiomeBaseShape_Cliff : public BiomeBaseShapeGenerator
{
public:
	BiomeBaseShape_Cliff(ApplicationState* appState);
	~BiomeBaseShape_Cliff();

	virtual bool ShowSettings() override;
	virtual void Update(GeneratorData* buffer, GeneratorTexture* seedTexture) override;
	virtual nlohmann::json OnSave() override;
	virtual void OnLoad(nlohmann::json data) override;

private:
	float m_Strength = 0.58f;
	int	  m_Seed = 0;
	float m_NoiseScale = 1.0f;
	float m_NoiseStrength = 1.0f;
	float m_DistortionScale = 1.4f;
	float m_DistortionStrength = 0.680f;
	float m_Smoothness = 0.03f;
	float m_Height = 1.0f;
	float m_Thickness = 0.5f;
	float m_Position = 0.0f;
	float m_Rotation = 0.0f;
	std::vector<int> m_SeedHistory;
};