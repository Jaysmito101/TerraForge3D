#include "Generators/GenerationManager.h"
#include "Data/ApplicationState.h"
#include "Data/ConfigManager.h"
#include "Profiler.h"
#include <exception>

namespace tf3d::generators
{

    FieldState::FieldState(ApplicationState *appState, GenerationDirtyManager *dirtyManager)
    {
        std::string configuredStorage;
        if (appState->configManager != nullptr &&
            appState->configManager->GetString("generation", "field_storage", configuredStorage)) {
            GeneratorData::SetDefaultStorage(
                configuredStorage == "R16F" ? GeneratorDataStorage::R16F : GeneratorDataStorage::R32F);
        }

        statistics            = std::make_shared<GeneratorDataStatistics>(appState);
        heightPyramid         = std::make_shared<HeightfieldPyramid>(appState);
        heightmapData         = std::make_shared<GeneratorData>();
        workingHeightmapData  = std::make_shared<GeneratorData>();
        swapBuffer            = std::make_shared<GeneratorData>();
        slopeGenerator        = std::make_shared<SlopeGenerator>(appState, appState->mainMap.tileResolution);
        workingSlopeGenerator = std::make_shared<SlopeGenerator>(appState, appState->mainMap.tileResolution);
        biomeMixer            = std::make_shared<BiomeMixer>(appState);
        biomeManagers.push_back(std::make_shared<BiomeManager>(appState, dirtyManager, "Default Global"));
    }

    UiState::UiState()
        : fieldStorageUiMode(GeneratorData::GetDefaultStorage() == GeneratorDataStorage::R16F ? 1 : 0)
    {
    }

    GenerationManager::GenerationManager(ApplicationState *appState)
        : m_AppState(appState), m_DirtyManager(), m_Field(appState, &m_DirtyManager), m_Ui()
    {
        m_AppState->eventManager->Subscribe("TileResolutionChanged", BIND_EVENT_FN(OnTileResolutionChange));
        m_AppState->eventManager->Subscribe("ForceUpdate", BIND_EVENT_FN(OnForceUpdate));

        // GenerationWorker invokes this callback on its thread with its shared OpenGL context current.
        m_Worker = std::make_unique<GenerationWorker>(
            "Generation Worker",
            [this](uint64_t requestId) { ExecuteActiveGeneration(requestId); });

        m_DirtyManager.MarkForce(GenerationDirtyCause::Force);
    }

    void GenerationManager::ExecuteActiveGeneration(uint64_t requestId)
    {
        if (m_ActiveGeneration == nullptr) {
            return;
        }

        const auto &snapshot = m_ActiveGeneration->snapshot;
        try {
            m_ActiveGeneration->result = ExecuteGeneration(snapshot, requestId);
        } catch (const std::exception &exception) {
            TF3D_LOG_ERROR("Generation request {} failed: {}", requestId, exception.what());
            m_ActiveGeneration->result = {requestId, snapshot.dirtyState.revision, false};
        } catch (...) {
            TF3D_LOG_ERROR("Generation request {} failed with an unknown exception", requestId);
            m_ActiveGeneration->result = {requestId, snapshot.dirtyState.revision, false};
        }
    }

    void GenerationManager::CompleteActiveGeneration(uint64_t requestId)
    {
        if (m_ActiveGeneration == nullptr) {
            TF3D_LOG_ERROR("Generation completed request {}, but GenerationManager has no active generation",
                           requestId);
            return;
        }

        const auto &result = m_ActiveGeneration->result;
        if (result.requestId != requestId) {
            TF3D_LOG_ERROR("Generation completed request {}, but the active generation has a result for request {}",
                           requestId, result.requestId);
            m_ActiveGeneration.reset();
            return;
        }

        if (result.producedOutput && IsCurrentGeneration(result)) {
            CommitHeightfield(m_ActiveGeneration->snapshot);
        }

        if (!result.producedOutput && !result.superseded) {
            const auto dirtyState = m_DirtyManager.Snapshot();
            if (dirtyState.revision == result.inputRevision) {
                m_LastFailedGenerationRevision = dirtyState.revision;
            }
        }
        m_ActiveGeneration.reset();
    }

    void GenerationManager::Update()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/update", PerformanceMonitor::Domain::Generation);
        if (m_Worker->HasContext()) {
            m_Worker->Poll();
            if (const auto completedRequestId = m_Worker->TryConsumeCompleted()) {
                CompleteActiveGeneration(*completedRequestId);
            }
        }
        SchedulePendingGeneration();
    }

    void GenerationManager::SchedulePendingGeneration()
    {
        const bool resizeRequiresGeneration = m_DirtyManager.HasCause(GenerationDirtyCause::Resize);
        if (m_Ui.updationPaused && !resizeRequiresGeneration) {
            return;
        }

        if (!m_DirtyManager.IsDirty() || m_ActiveGeneration != nullptr ||
            m_DirtyManager.Snapshot().revision == m_LastFailedGenerationRevision ||
            !m_Worker->CanAcceptRequest()) {
            return;
        }

        RequestGeneration();
    }

    void GenerationManager::MarkForRegeneration()
    {
        m_DirtyManager.MarkForce(GenerationDirtyCause::External);
    }

    bool GenerationManager::OnForceUpdate(const std::string &, void *)
    {
        MarkForRegeneration();
        return false;
    }

    void GenerationManager::RequestGeneration()
    {
        if (m_ActiveGeneration != nullptr || !m_Worker->CanAcceptRequest()) {
            return;
        }

        m_ActiveGeneration = std::make_unique<ActiveGeneration>(CaptureGenerationSnapshot());

        switch (m_Worker->Request().status) {
            case GenerationWorker::RequestStatus::Queued:
                // generation worker executes the callback on its thread
                return;
            case GenerationWorker::RequestStatus::Busy:
                m_ActiveGeneration.reset();
                return;
            case GenerationWorker::RequestStatus::Unavailable: {
                // without a worker context execute synchronously on the render thread.
                const uint64_t requestId = NextUniqueId();
                ExecuteActiveGeneration(requestId);
                CompleteActiveGeneration(requestId);
                return;
            }
        }
    }

    GenerationRequestSnapshot GenerationManager::CaptureGenerationSnapshot()
    {
        GenerationRequestSnapshot snapshot{};
        snapshot.tileResolution         = m_AppState->mainMap.tileResolution;
        snapshot.tileSize               = m_AppState->mainMap.tileSize;
        snapshot.dirtyState             = m_DirtyManager.Snapshot();
        snapshot.seedTexture            = m_Field.seedTexture;
        snapshot.workingHeightmapData   = m_Field.workingHeightmapData;
        snapshot.swapBuffer             = m_Field.swapBuffer;
        snapshot.slopeGenerator         = m_Field.workingSlopeGenerator;
        snapshot.statistics             = m_Field.statistics;
        snapshot.gpuWorkgroupSize       = m_AppState->constants.gpuWorkgroupSize;
        snapshot.statisticsSampleStride = m_Field.statisticsSampleStride;
        snapshot.biomes.clear();
        snapshot.biomes.reserve(m_Field.biomeManagers.size());
        if (m_Field.biomeMixer != nullptr) {
            snapshot.mixer        = m_Field.biomeMixer->GetState();
            snapshot.mixerRuntime = m_Field.biomeMixer->GetRuntime();
        }
        for (const auto &biome : m_Field.biomeManagers) {
            if (biome != nullptr) {
                auto biomeSnapshot = biome->CaptureSnapshot();
                snapshot.biomes.emplace_back(std::move(biomeSnapshot));
            } else {
                snapshot.biomes.emplace_back();
            }
        }
        return snapshot;
    }

    void GenerationManager::WaitForGenerationWorker()
    {
        m_Worker->WaitForIdle();
    }

#define TF3D_RETURN_IF_GENERATION_STALE(context, result, ...)                                       \
    do {                                                                                            \
        const bool tf3d_generation_is_current = (context).IsCurrent();                              \
        (result).superseded                   = (result).superseded || !tf3d_generation_is_current; \
        if (!tf3d_generation_is_current) {                                                          \
            return __VA_ARGS__;                                                                     \
        }                                                                                           \
    } while (false)

    GenerationExecutionResult GenerationManager::ExecuteGeneration(const GenerationRequestSnapshot &snapshot,
                                                                   uint64_t requestId)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/execute", PerformanceMonitor::Domain::Generation);
        GenerationExecutionResult result{requestId, snapshot.dirtyState.revision, false};
        const auto &dirtyState = snapshot.dirtyState;

        const GenerationContext context{snapshot.seedTexture.get(), snapshot.tileResolution,
                                        snapshot.gpuWorkgroupSize, snapshot.tileSize,
                                        &m_DirtyManager, dirtyState.revision};

        TF3D_RETURN_IF_GENERATION_STALE(context, result, result);
        bool biomeWorkRequested = false, biomeUpdateFailed = false;
        if (!ExecuteBiomeStage(snapshot, context, result, biomeWorkRequested, biomeUpdateFailed)) {
            return result;
        }

        const bool forceUpdate              = dirtyState.RequiresForce();
        const bool heightfieldWorkRequested = biomeWorkRequested ||
                                              dirtyState.Has(GenerationDirtyScope::Mixer) || forceUpdate;

        if (heightfieldWorkRequested && !biomeUpdateFailed) {
            TF3D_RETURN_IF_GENERATION_STALE(context, result, result);
            ExecuteHeightfieldStage(snapshot, context, result);
        }

        return result;
    }

    bool GenerationManager::ExecuteBiomeStage(const GenerationRequestSnapshot &snapshot,
                                              const GenerationContext &context,
                                              GenerationExecutionResult &result,
                                              bool &biomeWorkRequested,
                                              bool &biomeUpdateFailed) const
    {
        const auto &dirtyState     = snapshot.dirtyState;
        const bool forceUpdate     = dirtyState.RequiresForce();
        const bool updateAllBiomes = dirtyState.Has(GenerationDirtyScope::AllBiomes);

        const size_t biomeCount = snapshot.biomes.size();
        for (size_t biomeIndex = 0; biomeIndex < biomeCount; ++biomeIndex) {
            TF3D_RETURN_IF_GENERATION_STALE(context, result, false);

            const auto &biomeSnapshot = snapshot.biomes[biomeIndex];
            const auto &biomeState    = biomeSnapshot.state;
            if (biomeSnapshot.runtime.data == nullptr || (!biomeState.updateRequired && !updateAllBiomes && !forceUpdate)) {
                continue;
            }

            biomeWorkRequested = true;
            if (!BiomeManager::Execute(&biomeSnapshot, &context, snapshot.swapBuffer.get()) && biomeState.enabled) {
                biomeUpdateFailed = true;
            }
        }
        return true;
    }

    void GenerationManager::ExecuteHeightfieldStage(const GenerationRequestSnapshot &snapshot,
                                                    const GenerationContext &context,
                                                    GenerationExecutionResult &result)
    {
        bool producedOutput = BiomeMixer::Execute(&snapshot.mixer,
                                                  &snapshot.mixerRuntime,
                                                  &context,
                                                  snapshot.biomes,
                                                  snapshot.workingHeightmapData.get(),
                                                  snapshot.swapBuffer.get());

        TF3D_RETURN_IF_GENERATION_STALE(context, result);
        if (producedOutput) {
            producedOutput = snapshot.slopeGenerator != nullptr &&
                             snapshot.slopeGenerator->Compute(snapshot.workingHeightmapData.get(), &context);
        }
        TF3D_RETURN_IF_GENERATION_STALE(context, result);

        if (producedOutput && snapshot.statistics != nullptr) {
            TF3D_PROFILE_SCOPE_CHILD("statistics");
            TF3D_PROFILE_GPU_SCOPE_CHILD("gpu");
            snapshot.statistics->Compute(snapshot.workingHeightmapData.get(),
                                         snapshot.tileResolution,
                                         snapshot.statisticsSampleStride,
                                         true,
                                         -1.0f,
                                         snapshot.gpuWorkgroupSize);
            TF3D_RETURN_IF_GENERATION_STALE(context, result);
        }

        TF3D_RETURN_IF_GENERATION_STALE(context, result);
        if (producedOutput && snapshot.workingHeightmapData != nullptr) {
            snapshot.workingHeightmapData->GenerateMipmaps();
        }
        result.producedOutput = producedOutput;
    }

    bool GenerationManager::CommitHeightfield(const GenerationRequestSnapshot &snapshot)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/commit", PerformanceMonitor::Domain::Generation);
        if (!m_DirtyManager.ConsumeIfRevision(snapshot.dirtyState.revision)) {
            return false;
        }

        m_Field.heightmapData.swap(m_Field.workingHeightmapData);
        m_Field.slopeGenerator.swap(m_Field.workingSlopeGenerator);
        m_Field.terrainRevision.fetch_add(1, std::memory_order_release);
        m_Field.statisticsResult = snapshot.statistics != nullptr
                                       ? snapshot.statistics->Read()
                                       : GeneratorDataStatisticsResult{};
        if (m_Field.heightPyramid != nullptr) {
            m_Field.heightPyramid->Rebuild(m_Field.heightmapData.get());
        }
        for (const auto &biomeSnapshot : snapshot.biomes) {
            for (const auto &biome : m_Field.biomeManagers) {
                if (biome != nullptr && biome->GetBiomeID() == biomeSnapshot.runtime.id) {
                    biome->MarkProcessed(biomeSnapshot.state.revision);
                    break;
                }
            }
        }
        return true;
    }

    bool GenerationManager::IsCurrentGeneration(const GenerationExecutionResult &result) const
    {
        if (!result.producedOutput || result.inputRevision == 0 || m_AppState == nullptr) {
            return false;
        }

        const auto dirtyState = m_DirtyManager.Snapshot();
        return dirtyState.revision == result.inputRevision;
    }

    bool GenerationManager::OnTileResolutionChange(const std::string, void *)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/resize", PerformanceMonitor::Domain::Generation);
        WaitForGenerationWorker();
        m_ActiveGeneration.reset();
        m_Worker->TryConsumeCompleted();
        auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
        m_Field.heightmapData->Resize(size);
        m_Field.workingHeightmapData->Resize(size);
        m_Field.swapBuffer->Resize(size);
        m_Field.slopeGenerator->Resize(m_AppState->mainMap.tileResolution);
        m_Field.workingSlopeGenerator->Resize(m_AppState->mainMap.tileResolution);
        m_DirtyManager.MarkForce(GenerationDirtyCause::Resize);
        for (auto biome : m_Field.biomeManagers) {
            biome->Resize();
        }

        return false;
    }

} // namespace tf3d::generators
