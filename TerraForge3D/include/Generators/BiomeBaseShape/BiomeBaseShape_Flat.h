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
	float m_Height = 0.0f;
};