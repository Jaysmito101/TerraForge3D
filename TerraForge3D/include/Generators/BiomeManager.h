#pragma once

#include "BiomeBaseShape/BiomeBaseShapeGenerator.h"
#include "GeneratatorData.h"
#include "Base/Base.h"

class ApplicationState;

enum BiomeBaseShapeStyle
{
	BiomeBaseShapeStyle_Flat,
	BiomeBaseShapeStyle_Classic,
	BiomeBaseShapeStyle_Cliff,
	BiomeBaseShapeStyle_Cracks,
	BiomeBaseShapeStyle_Crater,
	BiomeBaseShapeStyle_Dunes,
	BiomeBaseShapeStyle_Volcano,
	BiomeBaseShapeStyle_SingleCrack,
	BiomeBaseShapeStyle_Count
};

static const char* s_BiomeBaseShapeStyleNames[] =
{
	"Flat",
	"Classic",
	"Cliff",
	"Cracks",
	"Crater",
	"Dunes",
	"Volcano",
	"SingleCrack"
};

#define BIOME_UI_PROPERTY(x) m_RequireUpdation = x || m_RequireUpdation

class BiomeManager
{
public:
	BiomeManager(ApplicationState* appState);
	~BiomeManager();

	void Resize();
	void Update(GeneratorData* swapBuffer);
	bool ShowSettings();

	inline const bool IsEnabled() const { return m_IsEnabled; }
	inline const char* GetBiomeName() const { return m_BiomeName; }
	inline const float GetCalculationTime() const { return m_CalculationTime; }
	inline const bool IsUpdationRequired() const { return m_RequireUpdation; }
	inline GeneratorData* GetBiomeData() const { return m_Data; }
	inline const ImVec4& GetColor() const { return m_Color; }

private:
	char m_BiomeName[64];
	bool m_IsEnabled = true;
	bool m_RequireUpdation = true;
	float m_CalculationTime = 0.0f;
	ImVec4 m_Color;
	std::string m_BiomeID = "";
	ApplicationState* m_AppState = nullptr;
	GeneratorData* m_Data = nullptr;
	std::vector<BiomeBaseShapeGenerator*> m_BaseShapeGenerators;
	int m_SelectedBaseShapeGenerator = 0;
};