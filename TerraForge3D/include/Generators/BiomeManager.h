#pragma once

#include "GeneratatorData.h"
#include "Base/Base.h"

class ApplicationState;

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
	inline GeneratorData* GetBiomeData() const { return m_BiomeData; }

private:
	char m_BiomeName[64];
	bool m_IsEnabled = true;
	bool m_RequireUpdation = false;
	float m_CalculationTime = 0.0f;
	std::string m_BiomeID = "";
	ApplicationState* m_AppState = nullptr;
	GeneratorData* m_BiomeData = nullptr;
	ComputeShader* m_Shader = nullptr;
};