#pragma once
#include "BiomeBaseShapeGenerator.h"

class BiomeBaseShape_Cracks : public BiomeBaseShapeGenerator
{
public:
	BiomeBaseShape_Cracks(ApplicationState* appState);
	~BiomeBaseShape_Cracks();

	virtual bool ShowSettings() override;
	virtual void Update(GeneratorData* buffer, GeneratorTexture* seedTexture) override;
	virtual nlohmann::json OnSave() override;
	virtual void OnLoad(nlohmann::json data) override;

private:
	float m_Strength = 0.58f;
	float m_Scale = 1.4f;
	int m_Seed = 0;
	float m_RandomHeights = 2.8f;
	float m_NoiseScale = 1.0f;
	float m_NoiseStrength = 1.0f;
	float m_CrackShapeDistortion = 0.680f;
	float m_Smoothness = 0.03f;
	bool m_SquareValue = false;
	bool m_AbsoluteValue = false;
	std::vector<int> m_SeedHistory;
	float m_Offset[3] = { 0.0f, 0.0f, 0.0f };
	float m_MinMaxHeight[2] = { 0.1f, 0.16f };
};