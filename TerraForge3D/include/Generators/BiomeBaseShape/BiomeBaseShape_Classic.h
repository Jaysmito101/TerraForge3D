#pragma once
#include "BiomeBaseShapeGenerator.h"

class BiomeBaseShape_Classic : public BiomeBaseShapeGenerator
{
public:
	BiomeBaseShape_Classic(ApplicationState* appState);
	~BiomeBaseShape_Classic();

	virtual bool ShowSettings() override;
	virtual void Update(GeneratorData* buffer, GeneratorTexture* seedTexture) override;
	virtual nlohmann::json OnSave() override;
	virtual void OnLoad(nlohmann::json data) override;

private:
	float m_Strength = 0.5f;
	float m_Scale = 1.0f;
	int m_Levels = 1;
	int m_Seed = 0;
	bool m_SquareValue = false;
	bool m_AbsoluteValue = false;
	std::vector<int> m_SeedHistory;
	float m_Offset[3] = { 0.0f, 0.0f, 0.0f };
};