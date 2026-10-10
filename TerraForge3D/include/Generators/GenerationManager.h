#pragma once
#include "Base/Base.h"
#include "Generators/BiomeManager.h"
#include "Generators/BiomeMixer.h"
#include "Generators/GenerationContext.h"
#include "Generators/GenerationDirtyManager.h"
#include "Generators/GenerationWorker.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorDataStatistics.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/HeightfieldPyramid.h"
#include "Generators/SlopeGenerator.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

TF3D_FWD_DEC_CLASS(ComputeShader, tf3d::base)
TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

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
        SelectedUINodeObjectType_CustomizeBaseShape,
        SelectedUINodeObjectType_CustomBaseShape = SelectedUINodeObjectType_CustomizeBaseShape,
        SelectedUINodeObjectType_MaskTool,
        SelectedUINodeObjectType_Filter,
        SelectedUINodeObjectType_Material
    };

    struct SelectedUINode {
        SelectedUINode() = default;

        explicit SelectedUINode(int biomeIndex,
                                SelectedUINodeObjectType objectType,
                                const char *nodeName)
            : m_BiomeIndex(biomeIndex), m_FilterIndex(-1), m_ObjectName(objectType),
              m_ID(MakeBiomeNodeID(biomeIndex, nodeName))
        {
        }

        static std::string MakeBiomeNodeID(int biomeIndex, const char *nodeName)
        {
            return std::to_string(biomeIndex) + "_Biome" + nodeName;
        }

        int m_BiomeIndex  = -1;
        int m_FilterIndex = -1;
        BiomeID m_BiomeID;
        SelectedUINodeObjectType m_ObjectName = SelectedUINodeObjectType_None;
        std::string m_ID;
    };

    struct FieldState {
        explicit FieldState(ApplicationState *appState, GenerationDirtyManager *dirtyManager);

        std::shared_ptr<GeneratorData> heightmapData;
        std::shared_ptr<GeneratorData> workingHeightmapData;
        std::shared_ptr<GeneratorData> swapBuffer;
        std::shared_ptr<GeneratorTexture> seedTexture;
        std::shared_ptr<SlopeGenerator> slopeGenerator;
        std::shared_ptr<SlopeGenerator> workingSlopeGenerator;
        std::shared_ptr<BiomeMixer> biomeMixer;
        std::shared_ptr<GeneratorDataStatistics> statistics;
        std::shared_ptr<HeightfieldPyramid> heightPyramid;
        std::vector<std::shared_ptr<BiomeManager>> biomeManagers;
        GeneratorDataStatisticsResult statisticsResult;
        std::atomic<uint64_t> terrainRevision = 0;
        int statisticsSampleStride            = 4;
    };

    struct UiState {
        explicit UiState(ApplicationState *appState);

        bool updationPaused             = false;
        bool useWorkerThread            = true;
        bool useSeedFromActiveMesh      = false;
        int32_t seedTextureResolution   = 256;
        int fieldStorageUiMode          = 0;
        bool fieldStorageRestartPending = false;
        SelectedUINode selectedNode;
    };

    struct GenerationRequestSnapshot {
        int32_t tileResolution = 0;
        float tileSize         = 1.0f;
        GenerationDirtyState dirtyState;
        std::shared_ptr<GeneratorTexture> seedTexture;
        std::shared_ptr<GeneratorData> workingHeightmapData;
        std::shared_ptr<GeneratorData> swapBuffer;
        std::shared_ptr<SlopeGenerator> slopeGenerator;
        std::shared_ptr<GeneratorDataStatistics> statistics;
        int32_t gpuWorkgroupSize       = 1;
        int32_t statisticsSampleStride = 4;
        std::vector<BiomeManager::Snapshot> biomes;
        BiomeMixer::Snapshot mixer;
        BiomeMixer::Runtime mixerRuntime;
    };

    struct GenerationExecutionResult {
        uint64_t requestId     = 0;
        uint64_t inputRevision = 0;
        bool producedOutput    = false;
        bool superseded        = false;
    };

    struct ActiveGeneration {
        explicit ActiveGeneration(GenerationRequestSnapshot snapshot)
            : snapshot(std::move(snapshot))
        {
        }

        GenerationRequestSnapshot snapshot;
        GenerationExecutionResult result;
    };

    class GenerationManager
    {
    public:
        GenerationManager(ApplicationState *appState);
        ~GenerationManager() = default;

        void Update();
        void MarkForRegeneration();
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
            return m_Field.terrainRevision.load(std::memory_order_acquire);
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

    private:
        bool OnForceUpdate(const std::string &params, void *paramsPtr);
        void WaitForGenerationWorker();
        void PullSeedTextureFromActiveMesh();
        void ShowSettingsInspector();
        void ShowSettingsDetailed();
        void ShowSettingsGlobalOptions();
        void ShowFieldStatistics();
        bool CommitHeightfield(const GenerationRequestSnapshot &snapshot);
        void SchedulePendingGeneration();
        void RequestGeneration();
        void ExecuteGenerationOnRenderThread();
        GenerationRequestSnapshot CaptureGenerationSnapshot();
        void ExecuteActiveGeneration(uint64_t requestId);
        void CompleteActiveGeneration(uint64_t requestId);
        GenerationExecutionResult ExecuteGeneration(const GenerationRequestSnapshot &snapshot,
                                                    uint64_t requestId);
        bool ExecuteBiomeStage(const GenerationRequestSnapshot &snapshot,
                               const GenerationContext &context,
                               GenerationExecutionResult &result,
                               bool &biomeWorkRequested,
                               bool &biomeUpdateFailed) const;
        void ExecuteHeightfieldStage(const GenerationRequestSnapshot &snapshot,
                                     const GenerationContext &context,
                                     GenerationExecutionResult &result);
        bool IsCurrentGeneration(const GenerationExecutionResult &result) const;

    private:
        ApplicationState *m_AppState = nullptr;

        GenerationDirtyManager m_DirtyManager;
        FieldState m_Field;
        UiState m_Ui;

        uint64_t m_LastFailedGenerationRevision = 0;
        std::unique_ptr<ActiveGeneration> m_ActiveGeneration;
        std::unique_ptr<GenerationWorker> m_Worker;
    };

} // namespace tf3d::generators
