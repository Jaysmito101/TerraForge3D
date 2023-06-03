#pragma once

#include "Generators/BiomeBaseShapeGenerator.h"
#include "Generators/DEMBaseShapeGenerator.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Base/Base.h"

class ApplicationState;

enum BiomeBaseShapeGeneratorMode
{
	BiomeBaseShapeGeneratorMode_Algorithm = 0,
	BiomeBaseShapeGeneratorMode_GlobalElevation,
	BiomeBaseShapeGeneratorMode_Count
};

static std::vector<std::string> s_BaseShapeGeneratorModeNames = { "Algorithm", "Global Elevation" };

#define BIOME_UI_PROPERTY(x) m_RequireUpdation = x || m_RequireUpdation

class BiomeManager
{
public:
	BiomeManager(ApplicationState* appState);
	~BiomeManager();

	void Resize();
	void Update(GeneratorData* swapBuffer, GeneratorTexture* seedTexture);
	// bool ShowSettings();
	bool ShowBaseShapeSettings();
	bool ShowCustomBaseShapeSettings();
	bool ShowGeneralSettings();
	bool ShowBaseNoiseSettings();

	inline const bool IsEnabled() const { return m_IsEnabled; }
	inline const char* GetBiomeName() const { return m_BiomeName; }
	inline const float GetCalculationTime() const { return m_CalculationTime; }
	inline const bool IsUpdationRequired() const { return m_RequireUpdation; }
	inline const bool IsUsingCustomBaseShape() const { return m_UseCustomBaseShape; }
	inline GeneratorData* GetBiomeData() const { return m_Data.get(); }
	inline const ImVec4& GetColor() const { return m_Color; }
	inline const int GetFiltersCount() const { return 0; }
	inline const std::vector<int>& GetFilters() const { return m_Filters; }
	inline const std::string& GetBiomeID() const { return m_BiomeID; }
	inline void SetName(const std::string& name) { strcpy(m_BiomeName, name.c_str()); }

	static bool AddBaseShapeGenerator(const std::string& config, ApplicationState* appState);
	static bool LoadBaseShapeGenerators(ApplicationState* appState);
	static void FreeBaseShapeGenerators();

private:
	char m_BiomeName[64];
	bool m_IsEnabled = true;
	bool m_UseCustomBaseShape = false;
	bool m_RequireUpdation = true;
	float m_CalculationTime = 0.0f;
	ImVec4 m_Color;
	std::string m_BiomeID = "";
	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<GeneratorData> m_Data;
	int32_t m_SelectedBaseShapeGenerator = 0;
	BiomeBaseShapeGeneratorMode m_SelectedBaseShapeGeneratorMode = BiomeBaseShapeGeneratorMode_Algorithm;
	std::vector<int> m_Filters;
	std::shared_ptr<DEMBaseShapeGenerator> m_DEMBaseShapeGenerator;

	static std::vector<std::shared_ptr<BiomeBaseShapeGenerator>> s_BaseShapeGenerators;
};