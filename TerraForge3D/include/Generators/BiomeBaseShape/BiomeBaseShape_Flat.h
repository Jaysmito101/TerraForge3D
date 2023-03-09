#pragma once
#include "BiomeBaseShapeGenerator.h"

class BiomeBaseShape_Flat : public BiomeBaseShapeGenerator
{
public:
	BiomeBaseShape_Flat(ApplicationState* appState);
	~BiomeBaseShape_Flat();

	virtual bool ShowSettings() override;
	virtual void Update(GeneratorData* buffer) override;
	virtual nlohmann::json OnSave() override;
	virtual void OnLoad(nlohmann::json data) override;

private:
	float m_Height = 0.5f;
	float m_Radius = 1.0f;
	float m_Frequency[2] = {1.0f, 1.0f};
	float m_Offset[2] = {0.0f, 0.0f};
	float m_Rotation = 0.0f;
	int m_SinWaveType = 0;
	int m_SubStyle = 0;
};