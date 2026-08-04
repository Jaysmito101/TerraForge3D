#pragma once

#include "Generators/BiomeBaseShapeGenerator.h"
#include "Generators/DEMBaseShapeGenerator.h"
#include "Generators/BiomeBaseNoiseGenerator.h"
#include "Generators/BiomeCustomBaseShape.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/CalculatedMaskGenerator.h"
#include "Generators/MaskTool.h"
#include "Generators/BiomeFilterStack.h"
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
	bool ShowMaskToolSettings();
	bool ShowFilterSettings(int filterIndex);

	inline const bool IsEnabled() const { return m_IsEnabled; }
	inline const char* GetBiomeName() const { return m_BiomeName; }
	inline const float GetCalculationTime() const { return m_CalculationTime; }
	inline const bool IsUpdationRequired() const { return m_RequireUpdation; }
	inline const bool IsUsingCustomBaseShape() const { return m_UseCustomBaseShape; }
	inline GeneratorData* GetBiomeData() const { return m_Data.get(); }
	inline const ImVec4& GetColor() const { return m_Color; }
	inline const int GetFiltersCount() const { return static_cast<int>(m_FilterStack->GetFilters().size()); }
	inline const std::vector<std::shared_ptr<BiomeFilter>>& GetFilters() const { return m_FilterStack->GetFilters(); }
	inline const std::vector<std::shared_ptr<BiomeFilterDefinition>>& GetFilterDefinitions() const { return m_FilterStack->GetDefinitions(); }
	inline const std::string& GetBiomeID() const { return m_BiomeID; }
	inline void SetName(const std::string& name) { strcpy(m_BiomeName, name.c_str()); }
	inline GeneratorTexture* GetMaskTexture() const { return m_MaskTool->GetTexture(); }
	inline GeneratorTexture* GetMaskPreviewTexture() const { return m_MaskTool->GetPreviewTexture(); }

	bool AddBaseShapeGenerator(const std::string& config);
	bool AddBaseShapeGenerator(const nlohmann::json& config, const std::string& source, const std::string& shaderPath);
	int AddFilter(const std::shared_ptr<BiomeFilterDefinition>& definition);
	bool RemoveFilter(int filterIndex);
	bool LoadUpResources();

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
	std::shared_ptr<BiomeFilterStack> m_FilterStack;
	std::shared_ptr<DEMBaseShapeGenerator> m_DEMBaseShapeGenerator;
	std::shared_ptr<CalculatedMaskGenerator> m_CalculatedMaskGenerator;

	std::shared_ptr<MaskTool> m_MaskTool;

	std::vector<std::shared_ptr<BiomeBaseShapeGenerator>> m_BaseShapeGenerators;
	std::shared_ptr<BiomeBaseNoiseGenerator> m_BaseNoiseGenerator;
	std::shared_ptr< BiomeCustomBaseShape> m_CustomBaseShape;
};
