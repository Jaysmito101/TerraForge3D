#pragma once
#include "Generators/GeneratatorData.h"
#include "Generators/BiomeManager.h"
#include "Base/Base.h"

class ApplicationState;
class ComputeShader;

class GenerationManager
{
public:
	GenerationManager(ApplicationState* appState);
	~GenerationManager();

	void Update();
	void ShowSettings();
	void InvalidateWorkInProgress();
	bool IsWorking();

	bool OnTileResolutionChange(const std::string params, void* paramsPtr);

	inline const bool IsUpdationPaused() const { return m_UpdationPaused; }
	inline void SetUpdationPaused(bool paused) { m_UpdationPaused = paused; }
	inline const bool IsWindowVisible() const { return m_IsWindowVisible; }
	inline void SetWindowVisible(bool visible) { m_IsWindowVisible = visible; }
	inline bool* IsWindowVisiblePtr() { return &m_IsWindowVisible; }
	inline GeneratorData* GetHeightmapData() const { return m_HeightmapData; }

	bool UpdateInternal(const std::string& params = "", void* paramsPtr = nullptr);

private:
	GeneratorData* m_HeightmapData = nullptr;
	GeneratorData* m_SwapBuffer = nullptr;
	ApplicationState* m_AppState = nullptr;
	ComputeShader* m_GenerationShader = nullptr;
	std::vector<BiomeManager*> m_BiomeManagers;
	bool m_IsWindowVisible = true;
	bool m_UpdationPaused = false;
	bool m_RequireUpdation = true;
	int m_SelectedBiome = -1;
};