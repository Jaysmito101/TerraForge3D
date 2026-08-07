#pragma once
#include "Base/Base.h"
#include "Generators/BiomeManager.h"
#include "Generators/BiomeMixer.h"
#include "Generators/GenerationWorker.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorDataStatistics.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/GenerationDirtyManager.h"
#include "Generators/HeightfieldPyramid.h"
#include "Generators/SlopeGenerator.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace tf3d::base
{
    class ComputeShader;
}
using tf3d::base::ComputeShader;
namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    enum SelectedUINodeObjectType {
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

    struct SelectedUINode {
        int m_BiomeIndex  = -1;
        int m_FilterIndex = -1;
        std::string m_BiomeID;
        SelectedUINodeObjectType m_ObjectName = SelectedUINodeObjectType_None;
        std::string m_ID;
    };

    struct FieldState {
        std::shared_ptr<GeneratorData> heightmapData;
        std::shared_ptr<GeneratorData> workingHeightmapData;
        std::shared_ptr<GeneratorData> swapBuffer;
        std::shared_ptr<GeneratorTexture> seedTexture;
        std::shared_ptr<SlopeGenerator> slopeGenerator;
        std::shared_ptr<BiomeMixer> biomeMixer;
        std::shared_ptr<GeneratorDataStatistics> statistics;
        std::shared_ptr<HeightfieldPyramid> heightPyramid;
        std::vector<std::shared_ptr<BiomeManager>> biomeManagers;
        GeneratorDataStatisticsResult statisticsResult;
        int statisticsSampleStride = 4;
    };

    struct UiState {
        bool updationPaused             = false;
        bool useSeedFromActiveMesh      = false;
        int32_t seedTextureResolution   = 256;
        int fieldStorageUiMode          = 0;
        bool fieldStorageRestartPending = false;
        SelectedUINode selectedNode;
    };

    struct GenerationRequestSnapshot {
        uint64_t requestId       = 0;
        uint64_t submittedFrame  = 0;
        uint64_t terrainRevision = 0;
        int32_t tileResolution   = 0;
        uint32_t biomeCount      = 0;
        uint32_t filterCount     = 0;
        bool force               = false;
        GenerationDirtyState dirtyState;
    };

#define MakeUINodeID(index1, objectname) (std::to_string(index1) + std::string("_Biome") + std::string(#objectname))

#define SetUINodeData(index, objectname)                                         \
    {                                                                            \
        m_Ui.selectedNode.m_BiomeIndex  = index;                                 \
        m_Ui.selectedNode.m_FilterIndex = -1;                                    \
        m_Ui.selectedNode.m_ID          = MakeUINodeID(index, objectname);       \
        m_Ui.selectedNode.m_ObjectName  = SelectedUINodeObjectType_##objectname; \
    }

    class GenerationManager
    {
    public:
        GenerationManager(ApplicationState *appState);
        ~GenerationManager();

        void Update();
        void ShowSettings();

        bool OnTileResolutionChange(const std::string params, void *paramsPtr);

        inline const bool IsUpdationPaused() const
        {
            return m_Ui.updationPaused;
        }
        inline void SetUpdationPaused(bool paused)
        {
            m_Ui.updationPaused = paused;
        }
        bool IsWindowVisible() const;
        void SetWindowVisible(bool visible);
        bool *IsWindowVisiblePtr();
        inline GeneratorData *GetHeightmapData() const
        {
            return m_Field.heightmapData.get();
        }
        inline HeightfieldPyramid *GetHeightPyramid() const
        {
            return m_Field.heightPyramid.get();
        }
        inline uint64_t GetTerrainRevision() const
        {
            return m_TerrainRevision.load(std::memory_order_acquire);
        }
        inline GeneratorTexture *GetSlopeTexture() const
        {
            return m_Field.slopeGenerator != nullptr ? m_Field.slopeGenerator->GetTexture() : nullptr;
        }
        inline bool HasSlopeTexture() const
        {
            return m_Field.slopeGenerator != nullptr && m_Field.slopeGenerator->IsReady() &&
                   !m_Worker->IsRunning() && !m_Worker->IsRequestPending();
        }
        inline const GeneratorDataStatisticsResult &GetFieldStatisticsResult() const
        {
            return m_Field.statisticsResult;
        }
        bool UpdateInternal(const std::string &params = "", void *paramsPtr = nullptr);
        inline const std::vector<std::shared_ptr<BiomeManager>> &GetBiomeManagers() const
        {
            return m_Field.biomeManagers;
        }

    private:
        void WaitForGenerationWorker();
        void PullSeedTextureFromActiveMesh();
        void ShowSettingsInspector();
        void ShowSettingsDetailed();
        void ShowSettingsGlobalOptions();
        void ShowFieldStatistics();
        void UpdateFieldStatistics();
        void GenerateHeightmapMipmaps();
        void CommitHeightfield();
        void RequestGeneration();
        GenerationRequestSnapshot CaptureGenerationSnapshot();
        GenerationRequestSnapshot TakeGenerationSnapshot();
        void StoreGenerationSnapshot(const GenerationRequestSnapshot &snapshot);
        void ExecuteGeneration();

    private:
        ApplicationState *m_AppState = nullptr;
        FieldState m_Field;
        UiState m_Ui;

        std::unique_ptr<GenerationWorker> m_Worker;
        std::atomic<uint64_t> m_TerrainRevision = 0;
        mutable std::mutex m_RequestSnapshotMutex;
        std::optional<GenerationRequestSnapshot> m_PendingGenerationSnapshot;
    };

} // namespace tf3d::generators
using tf3d::generators::FieldState;
using tf3d::generators::GenerationManager;
using tf3d::generators::GenerationRequestSnapshot;
using tf3d::generators::SelectedUINode;
using tf3d::generators::UiState;
