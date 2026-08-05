#pragma once
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/BiomeManager.h"
#include "Generators/BiomeMixer.h"
#include "Generators/GeneratorDataStatistics.h"
#include "Base/Base.h"

#include <array>

class ApplicationState;
class ComputeShader;
struct GLFWwindow;

enum SelectedUINodeObjectType
{
	SelectedUINodeObjectType_None = 0,
	SelectedUINodeObjectType_GlobalOptions,
	SelectedUINodeObjectType_GlobalBiomeMixer,
	SelectedUINodeObjectType_General,
	SelectedUINodeObjectType_Filters,
	SelectedUINodeObjectType_BaseNoise,
	SelectedUINodeObjectType_BaseShape,
	SelectedUINodeObjectType_CustomBaseShape,
	SelectedUINodeObjectType_MaskTool,
	SelectedUINodeObjectType_Filter,
	SelectedUINodeObjectType_Material
};

struct SelectedUINode
{
	int m_BiomeIndex = -1;
	int m_FilterIndex = -1;
	std::string m_BiomeID;
	SelectedUINodeObjectType m_ObjectName;
	std::string m_ID;
};

#define MakeUINodeID(index1, objectname) (std::to_string(index1) + std::string("_Biome") + std::string(#objectname))


#define SetUINodeData(index, objectname) \
{ \
	m_SelectedNodeUI.m_BiomeIndex = index; \
	m_SelectedNodeUI.m_FilterIndex = -1; \
	m_SelectedNodeUI.m_ID = MakeUINodeID(index, objectname); \
	m_SelectedNodeUI.m_ObjectName = SelectedUINodeObjectType_##objectname; \
}

class GenerationManager
{
public:
	GenerationManager(ApplicationState* appState);
	~GenerationManager();

	void Update();
	void ShowSettings();

	bool OnTileResolutionChange(const std::string params, void* paramsPtr);

	inline const bool IsUpdationPaused() const { return m_UpdationPaused; }
	inline void SetUpdationPaused(bool paused) { m_UpdationPaused = paused; }
	inline const bool IsWindowVisible() const { return m_IsWindowVisible; }
	inline void SetWindowVisible(bool visible) { m_IsWindowVisible = visible; }
	inline bool* IsWindowVisiblePtr() { return &m_IsWindowVisible; }
	inline GeneratorData* GetHeightmapData() const { return m_HeightmapData.get(); }
	inline const GeneratorDataStatisticsResult& GetFieldStatisticsResult() const { return m_FieldStatisticsResult; }
	bool UpdateInternal(const std::string& params = "", void* paramsPtr = nullptr);
	inline const std::vector<std::shared_ptr<BiomeManager>>& GetBiomeManagers() const { return m_BiomeManagers; }

private:
	void WaitForGenerationWorker();
	void PullSeedTextureFromActiveMesh();
	void ShowSettingsInspector();
	void ShowSettingsDetailed();
	void ShowSettingsGlobalOptions();
	void ShowFieldStatistics();
	void UpdateFieldStatistics();
	void GenerateHeightmapMipmaps();
	void RequestGeneration(bool force);
	void GenerationWorkerLoop();
	void ExecuteGeneration(bool force);

private:
	ApplicationState* m_AppState = nullptr;

	std::shared_ptr<GeneratorData> m_HeightmapData;
	std::shared_ptr<GeneratorData> m_WorkingHeightmapData;
	std::shared_ptr<GeneratorData> m_SwapBuffer;
	std::shared_ptr<GeneratorTexture> m_SeedTexture;
	std::shared_ptr<BiomeMixer> m_BiomeMixer;
	std::shared_ptr<GeneratorDataStatistics> m_FieldStatistics;

	std::vector<std::shared_ptr<BiomeManager>> m_BiomeManagers;

	bool m_IsWindowVisible = true;
	bool m_UpdationPaused = false;
	std::atomic_bool m_RequireUpdation = true;
	bool m_UseSeedFromActiveMesh = false;

	GLFWwindow* m_GenerationWindow = nullptr;
	std::thread m_GenerationWorker;
	std::mutex m_GenerationMutex;
	std::condition_variable m_GenerationCondition;
	std::atomic_bool m_GenerationRequestPending = false;
	bool m_GenerationForceRequested = false;
	std::atomic_bool m_GenerationRunning = false;
	std::atomic_bool m_GenerationCompleted = false;
	std::atomic_bool m_StopGenerationWorker = false;

	int32_t m_SeedTextureResolution = 256;
	int m_FieldStorageUiMode = 0;
	bool m_FieldStorageRestartPending = false;
	GeneratorDataStatisticsResult m_FieldStatisticsResult;
	int m_FieldStatisticsSampleStride = 4;
	SelectedUINode m_SelectedNodeUI;
};
