#pragma once
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/BiomeManager.h"
#include "Base/Base.h"

class ApplicationState;
class ComputeShader;

enum SelectedUINodeObjectType
{
	SelectedUINodeObjectType_None = 0,
	SelectedUINodeObjectType_GlobalOptions,
	SelectedUINodeObjectType_General,
	SelectedUINodeObjectType_Filters,
	SelectedUINodeObjectType_BaseNoise,
	SelectedUINodeObjectType_BaseShape,
	SelectedUINodeObjectType_CustomBaseShape,
	SelectedUINodeObjectType_Material
};

struct SelectedUINode
{
	int m_BiomeIndex;
	std::string m_BiomeID;
	SelectedUINodeObjectType m_ObjectName;
	std::string m_ID;
};

#define MakeUINodeID(index1, objectname) (std::to_string(index1) + std::string("_Biome") + ###objectname )


#define SetUINodeData(index, objectname) \
{ \
	m_SelectedNodeUI.m_BiomeIndex = index; \
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
	bool UpdateInternal(const std::string& params = "", void* paramsPtr = nullptr);

private:
	void PullSeedTextureFromActiveMesh();
	void ShowSettingsInspector();
	void ShowSettingsDetailed();
	void ShowSettingsGlobalOptions();

private:
	ApplicationState* m_AppState = nullptr;
	std::shared_ptr<GeneratorData> m_HeightmapData;
	std::shared_ptr<GeneratorData> m_SwapBuffer;
	std::shared_ptr<GeneratorTexture> m_SeedTexture;
	// ComputeShader* m_BlurrShader = nullptr;
	std::vector<std::shared_ptr<BiomeManager>> m_BiomeManagers;
	bool m_IsWindowVisible = true;
	bool m_UpdationPaused = false;
	bool m_RequireUpdation = true;
	bool m_UseSeedFromActiveMesh = false;
	int32_t m_SeedTextureResolution = 256;
	SelectedUINode m_SelectedNodeUI;
};