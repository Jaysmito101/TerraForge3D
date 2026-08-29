#include "Generators/GenerationManager.h"
#include "Base/Shader.h"
#include "Data/ApplicationState.h"
#include "Data/ConfigManager.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"
#include <algorithm>
#include <map>

namespace tf3d::generators
{

    GenerationManager::GenerationManager(ApplicationState *appState)
    {
        // if (!BiomeManager::LoadBaseShapeGenerators(appState)) Log("Failed to load Base Shape Generators!");
        m_AppState = appState;
        TF3D_PROFILE_SET_METADATA("terrain/tile-resolution",
                                  std::to_string(m_AppState->mainMap.tileResolution));
        TF3D_PROFILE_SET_METADATA("terrain/revision", "0");
        std::string configuredStorage;
        if (m_AppState->configManager != nullptr && m_AppState->configManager->GetString("generation", "field_storage", configuredStorage)) {
            GeneratorData::SetDefaultStorage(configuredStorage == "R16F" ? GeneratorDataStorage::R16F : GeneratorDataStorage::R32F);
        }
        m_Ui.fieldStorageUiMode = GeneratorData::GetDefaultStorage() == GeneratorDataStorage::R16F ? 1 : 0;
        m_Field.statistics      = std::make_shared<GeneratorDataStatistics>(m_AppState);
        m_Field.heightPyramid   = std::make_shared<HeightfieldPyramid>(m_AppState);
        m_AppState->eventManager->Subscribe("TileResolutionChanged", BIND_EVENT_FN(OnTileResolutionChange));
        m_AppState->eventManager->Subscribe("ForceUpdate", BIND_EVENT_FN(UpdateInternal));
        m_Field.heightmapData         = std::make_shared<GeneratorData>();
        m_Field.workingHeightmapData  = std::make_shared<GeneratorData>();
        m_Field.swapBuffer            = std::make_shared<GeneratorData>();
        m_Field.slopeGenerator        = std::make_shared<SlopeGenerator>(m_AppState, m_AppState->mainMap.tileResolution);
        m_Field.workingSlopeGenerator = std::make_shared<SlopeGenerator>(m_AppState, m_AppState->mainMap.tileResolution);
        m_Field.biomeMixer            = std::make_shared<BiomeMixer>(m_AppState);
        m_Field.biomeManagers.push_back(std::make_shared<BiomeManager>(m_AppState));
        m_Field.biomeManagers.back()->SetName("Default Global");
        TF3D_PROFILE_SET_METADATA("terrain/biome-count",
                                  std::to_string(m_Field.biomeManagers.size()));
        m_Worker = std::make_unique<GenerationWorker>("Generation Worker", [this](bool, uint64_t requestId) {
            StoreCompletedGenerationResult(ExecuteGeneration(requestId));
        });
        m_AppState->generationDirtyManager.MarkForce(GenerationDirtyCause::Force);
    }

    GenerationManager::~GenerationManager() = default;

    bool GenerationManager::IsWindowVisible() const
    {
        return m_AppState != nullptr && m_AppState->windows.generationManager;
    }

    void GenerationManager::SetWindowVisible(bool visible)
    {
        if (m_AppState != nullptr)
            m_AppState->windows.generationManager = visible;
    }

    bool *GenerationManager::IsWindowVisiblePtr()
    {
        return m_AppState != nullptr ? &m_AppState->windows.generationManager : nullptr;
    }

    void GenerationManager::Update()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/update", PerformanceMonitor::Domain::Generation);
        const bool resolutionGenerationPending =
            m_ResolutionGenerationPending.load(std::memory_order_acquire);
        if (m_Ui.updationPaused && !resolutionGenerationPending)
            return;
        if (!m_Worker->HasContext()) {
            if (m_AppState->generationDirtyManager.IsDirty() || resolutionGenerationPending) {
                m_ResolutionGenerationPending.store(false, std::memory_order_release);
                RequestGeneration();
            }
            return;
        }

        m_Worker->PollCompletion();
        if (m_Worker->IsCompleted() && !m_Worker->IsRunning()) {
            if (m_Worker->ConsumeCompleted()) {
                const uint64_t requestId = m_Worker->GetCompletedRequestId();
                TF3D_PROFILE_SCOPE_FLOW("generation/publish", PerformanceMonitor::Domain::Generation, requestId);
                const auto result  = TakeCompletedGenerationResult();
                const bool current = result.producedOutput && IsCurrentGeneration(result);
                if (current && CommitHeightfield(result.inputRevision)) {
                    TF3D_PROFILE_FLOW_STEP_DOMAIN("generation/request", requestId, "published",
                                                  PerformanceMonitor::Domain::Generation);
                    TF3D_PROFILE_FLOW_END_DOMAIN("generation/request", requestId, PerformanceMonitor::Domain::Generation);
                } else if (result.producedOutput) {
                    TF3D_PROFILE_FLOW_STEP_DOMAIN("generation/request", requestId, "superseded",
                                                  PerformanceMonitor::Domain::Generation);
                    TF3D_PROFILE_FLOW_END_DOMAIN("generation/request", requestId, PerformanceMonitor::Domain::Generation);
                } else {
                    TF3D_PROFILE_FLOW_STEP_DOMAIN("generation/request", requestId, "no-output",
                                                  PerformanceMonitor::Domain::Generation);
                    TF3D_PROFILE_FLOW_END_DOMAIN("generation/request", requestId, PerformanceMonitor::Domain::Generation);
                }
            }
        }

        if ((m_AppState->generationDirtyManager.IsDirty() || resolutionGenerationPending) &&
            !m_Worker->IsRunning() && !m_Worker->IsRequestPending()) {
            m_ResolutionGenerationPending.store(false, std::memory_order_release);
            RequestGeneration();
        }
    }

    bool GenerationManager::UpdateInternal(const std::string &, void *)
    {
        m_AppState->generationDirtyManager.MarkForce(GenerationDirtyCause::External);
        return false;
    }

    void GenerationManager::RequestGeneration()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/request", PerformanceMonitor::Domain::Generation);
        GenerationRequestSnapshot snapshot = CaptureGenerationSnapshot();
        TF3D_PROFILE_SET_METADATA("terrain/tile-resolution",
                                  std::to_string(snapshot.tileResolution));
        TF3D_PROFILE_SET_METADATA("terrain/revision",
                                  std::to_string(snapshot.terrainRevision));
        TF3D_PROFILE_SET_METADATA("terrain/biome-count",
                                  std::to_string(snapshot.biomeCount));
        TF3D_PROFILE_SET_METADATA("terrain/filter-count",
                                  std::to_string(snapshot.filterCount));
        const bool force = snapshot.dirtyState.RequiresForce();
        StoreGenerationSnapshot(std::move(snapshot));

        uint64_t requestId = 0;
        if (!m_Worker->Request(force, &requestId)) {
            requestId = NextUniqueId();
            TF3D_PROFILE_FLOW_BEGIN_DOMAIN("generation/request", requestId, PerformanceMonitor::Domain::Generation);
            TF3D_PROFILE_VALUE_DOMAIN_FLOW("generation/request/force", force ? 1 : 0, 0, 0,
                                           PerformanceMonitor::Domain::Generation, requestId);
            TF3D_PROFILE_FLOW_STEP_DOMAIN("generation/request", requestId, "started",
                                          PerformanceMonitor::Domain::Generation);
            TF3D_PROFILE_INSTANT_DOMAIN_FLOW("generation/request/fallback", PerformanceMonitor::Domain::Generation, requestId);
            const auto result    = ExecuteGeneration(requestId);
            const bool committed = result.producedOutput && IsCurrentGeneration(result) &&
                                   CommitHeightfield(result.inputRevision);
            if (committed) {
                TF3D_PROFILE_SCOPE_FLOW("generation/publish", PerformanceMonitor::Domain::Generation, requestId);
                TF3D_PROFILE_FLOW_STEP_DOMAIN("generation/request", requestId, "published",
                                              PerformanceMonitor::Domain::Generation);
                TF3D_PROFILE_FLOW_END_DOMAIN("generation/request", requestId, PerformanceMonitor::Domain::Generation);
            } else if (result.producedOutput) {
                TF3D_PROFILE_FLOW_STEP_DOMAIN("generation/request", requestId, "superseded",
                                              PerformanceMonitor::Domain::Generation);
                TF3D_PROFILE_FLOW_END_DOMAIN("generation/request", requestId, PerformanceMonitor::Domain::Generation);
            }
        }
    }

    GenerationRequestSnapshot GenerationManager::CaptureGenerationSnapshot()
    {
        GenerationRequestSnapshot snapshot;
        snapshot.submittedFrame  = TF3D_PROFILE_CURRENT_FRAME_ID();
        snapshot.terrainRevision = m_TerrainRevision.load(std::memory_order_acquire);
        snapshot.tileResolution  = m_AppState->mainMap.tileResolution;
        snapshot.tileSize        = m_AppState->mainMap.tileSize;
        snapshot.dirtyState      = m_AppState->generationDirtyManager.Snapshot();
        CaptureGenerationState(snapshot);
        return snapshot;
    }

    void GenerationManager::CaptureGenerationState(GenerationRequestSnapshot &snapshot) const
    {
        snapshot.seedTexture          = m_Field.seedTexture;
        snapshot.workingHeightmapData = m_Field.workingHeightmapData;
        snapshot.swapBuffer           = m_Field.swapBuffer;
        snapshot.slopeGenerator       = m_Field.workingSlopeGenerator;
        snapshot.gpuWorkgroupSize     = m_AppState->constants.gpuWorkgroupSize;
        snapshot.biomes.clear();
        snapshot.biomes.reserve(m_Field.biomeManagers.size());
        snapshot.biomeCount  = static_cast<uint32_t>(m_Field.biomeManagers.size());
        snapshot.filterCount = 0;
        if (m_Field.biomeMixer != nullptr) {
            snapshot.mixer        = m_Field.biomeMixer->GetState();
            snapshot.mixerRuntime = m_Field.biomeMixer->GetRuntime();
        }
        for (const auto &biome : m_Field.biomeManagers) {
            if (biome != nullptr) {
                auto biomeSnapshot = biome->CaptureSnapshot();
                snapshot.filterCount += biomeSnapshot.state.filters.has_value()
                                            ? static_cast<uint32_t>(biomeSnapshot.state.filters->value.filters.size())
                                            : 0;
                snapshot.biomes.emplace_back(std::move(biomeSnapshot));
            } else {
                snapshot.biomes.emplace_back();
            }
        }
    }

    void GenerationManager::StoreGenerationSnapshot(GenerationRequestSnapshot snapshot)
    {
        std::lock_guard lock(m_RequestSnapshotMutex);
        m_PendingGenerationSnapshot = std::move(snapshot);
    }

    GenerationRequestSnapshot GenerationManager::TakeGenerationSnapshot(uint64_t requestId)
    {
        GenerationRequestSnapshot snapshot;
        {
            std::lock_guard lock(m_RequestSnapshotMutex);
            if (m_PendingGenerationSnapshot.has_value()) {
                snapshot = std::move(*m_PendingGenerationSnapshot);
                m_PendingGenerationSnapshot.reset();
            }
        }
        if (snapshot.requestId == 0)
            snapshot.requestId = requestId;
        return snapshot;
    }

    void GenerationManager::WaitForGenerationWorker()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/wait-for-worker", PerformanceMonitor::Domain::Wait);
        m_Worker->WaitForIdle();
    }

    GenerationExecutionResult GenerationManager::ExecuteGeneration(uint64_t requestId)
    {
        const GenerationRequestSnapshot snapshot = TakeGenerationSnapshot(requestId);
        GenerationExecutionResult result{snapshot.requestId, snapshot.dirtyState.revision, false};
        const auto &dirtyState = snapshot.dirtyState;
        TF3D_PROFILE_SCOPE_FLOW("generation", PerformanceMonitor::Domain::Generation, snapshot.requestId);
        TF3D_PROFILE_VALUE_DOMAIN_FLOW("generation/request-snapshot", dirtyState.mask, dirtyState.causeMask,
                                       dirtyState.revision, PerformanceMonitor::Domain::Generation, snapshot.requestId);
        TF3D_PROFILE_VALUE_DOMAIN_FLOW("generation/request-context", snapshot.submittedFrame,
                                       snapshot.terrainRevision, snapshot.requestId,
                                       PerformanceMonitor::Domain::Generation, snapshot.requestId);
        TF3D_PROFILE_VALUE_DOMAIN_FLOW("generation/request-shape", static_cast<uint64_t>(snapshot.tileResolution),
                                       snapshot.biomeCount, snapshot.filterCount,
                                       PerformanceMonitor::Domain::Generation, snapshot.requestId);
        TF3D_PROFILE_VALUE_DOMAIN_FLOW("generation/dirty-state", dirtyState.mask, dirtyState.causeMask,
                                       dirtyState.revision, PerformanceMonitor::Domain::Generation, snapshot.requestId);

        const bool forceUpdate     = dirtyState.RequiresForce();
        const bool updateBiomes    = dirtyState.Has(GenerationDirtyScope::Biomes);
        const bool updateAllBiomes = dirtyState.Has(GenerationDirtyScope::AllBiomes);
        const GenerationContext context{snapshot.seedTexture.get(), snapshot.tileResolution,
                                        snapshot.gpuWorkgroupSize, snapshot.tileSize};
        const size_t biomeCount = snapshot.biomes.size();
        bool biomeWorkRequested = false;
        bool biomeUpdateFailed  = false;
        for (size_t biomeIndex = 0; biomeIndex < biomeCount; ++biomeIndex) {
            const auto &biomeSnapshot = snapshot.biomes[biomeIndex];
            const auto &biomeState    = biomeSnapshot.state;
            if (biomeSnapshot.runtime.data != nullptr &&
                (biomeState.updateRequired || updateBiomes || updateAllBiomes || forceUpdate)) {
                TF3D_PROFILE_SCOPE_CHILD("biome");
                biomeWorkRequested = true;
                if (!BiomeManager::Execute(&biomeSnapshot,
                                           &context,
                                           snapshot.swapBuffer.get()) &&
                    biomeState.enabled) {
                    biomeUpdateFailed = true;
                }
            }
        }
        if (biomeWorkRequested || dirtyState.Has(GenerationDirtyScope::Mixer) || forceUpdate) {
            if (biomeUpdateFailed) {
                return result;
            }
            bool producedOutput = false;
            {
                TF3D_PROFILE_SCOPE_CHILD("mixer");
                producedOutput = BiomeMixer::Execute(&snapshot.mixer,
                                                     &snapshot.mixerRuntime,
                                                     &context,
                                                     snapshot.biomes,
                                                     snapshot.workingHeightmapData.get(),
                                                     snapshot.swapBuffer.get());
            }
            if (producedOutput) {
                TF3D_PROFILE_SCOPE_CHILD("slope");
                producedOutput = snapshot.slopeGenerator != nullptr &&
                                 snapshot.slopeGenerator->Compute(snapshot.workingHeightmapData.get(), &context);
            }
            result.producedOutput = producedOutput;
            return result;
        }
        return result;
    }

    void GenerationManager::StoreCompletedGenerationResult(GenerationExecutionResult result)
    {
        std::lock_guard lock(m_ResultMutex);
        m_CompletedGenerationResult = result;
    }

    GenerationExecutionResult GenerationManager::TakeCompletedGenerationResult()
    {
        std::lock_guard lock(m_ResultMutex);
        if (!m_CompletedGenerationResult.has_value()) {
            return {};
        }

        GenerationExecutionResult result = *m_CompletedGenerationResult;
        m_CompletedGenerationResult.reset();
        return result;
    }

    bool GenerationManager::IsCurrentGeneration(const GenerationExecutionResult &result) const
    {
        if (!result.producedOutput || result.inputRevision == 0 || m_AppState == nullptr) {
            return false;
        }

        const auto dirtyState = m_AppState->generationDirtyManager.Snapshot();
        return dirtyState.revision == result.inputRevision;
    }

    void GenerationManager::PullSeedTextureFromActiveMesh()
    {
        if (!m_Ui.useSeedFromActiveMesh)
            return;
        m_Field.seedTexture->MakeCPUCopy();
        m_Field.seedTexture->ZeroCPUCopy();
        auto mesh = m_AppState->mainModel->mesh;
        for (auto i = 0; i < mesh->GetVertexCount(); i++) {
            const auto &vertex = mesh->GetVertex(i);
            m_Field.seedTexture->SetPixel(vertex.texCoord.x, vertex.texCoord.y, vertex.position.x, vertex.position.z, vertex.position.y);
        }
        m_Field.seedTexture->UploadCPUCopy();
        m_Field.seedTexture->FreeCPUCopy();
    }

    void GenerationManager::ShowSettings()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/ui", PerformanceMonitor::Domain::Ui);
        ShowSettingsInspector();
        ShowSettingsDetailed();
    }

    void GenerationManager::ShowSettingsInspector()
    {
        ImGui::Begin("Generator Inspector", &m_AppState->windows.generationManager);

        if (ImGui::Selectable("Options", m_Ui.selectedNode.m_ID == "GlobalOptions")) {
            m_Ui.selectedNode.m_ID         = "GlobalOptions";
            m_Ui.selectedNode.m_ObjectName = SelectedUINodeObjectType_GlobalOptions;
        }

        if (ImGui::Selectable("Biome Mixer", m_Ui.selectedNode.m_ID == "GlobalBiomeMixer")) {
            m_Ui.selectedNode.m_ID         = "GlobalBiomeMixer";
            m_Ui.selectedNode.m_ObjectName = SelectedUINodeObjectType_GlobalBiomeMixer;
        }

        const bool biomesOpen = ImGui::TreeNodeEx("Biomes", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
        ImGui::SameLine();
        if (ImGui::Button("Add##BiomeAdd")) {
            m_Field.biomeManagers.push_back(std::make_shared<BiomeManager>(m_AppState));
            m_Field.biomeManagers.back()->SetName("Biome " + std::to_string(m_Field.biomeManagers.size()));
            m_AppState->generationDirtyManager.MarkMixer();
        }
        if (biomesOpen) {
            if (m_Field.biomeManagers.size() == 0)
                ImGui::Text("No Biomes Added!");
            for (int i = 0; i < m_Field.biomeManagers.size(); i++) {
                auto biome = m_Field.biomeManagers[i];
                ImGui::PushID(biome->GetBiomeID().c_str());
                const bool biomeOpen = ImGui::TreeNodeEx(biome->GetBiomeName(), ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
                ImGui::SameLine();
                bool deleteBiome = false;
                if (ImGui::Button("Delete")) {
                    TF3D_LOG_DEBUG("Loaded {} biome managers", m_Field.biomeManagers.size());
                    m_Field.biomeManagers.erase(m_Field.biomeManagers.begin() + i);
                    TF3D_LOG_DEBUG("Active biome managers: {}", m_Field.biomeManagers.size());
                    m_AppState->generationDirtyManager.MarkBiomes();
                    m_AppState->generationDirtyManager.MarkMixer();
                    SetUINodeData(-1, None);
                    deleteBiome = true;
                }
                if (deleteBiome) {
                    if (biomeOpen) {
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                    break;
                }
                if (biomeOpen) {
                    if (ImGui::Selectable("General", m_Ui.selectedNode.m_ID == MakeUINodeID(i, General))) {
                        SetUINodeData(i, General);
                    }
                    if (ImGui::Selectable("Mask", m_Ui.selectedNode.m_ID == MakeUINodeID(i, MaskTool))) {
                        SetUINodeData(i, MaskTool);
                    }
                    if (ImGui::Selectable("Base Shape", m_Ui.selectedNode.m_ID == MakeUINodeID(i, BaseShape))) {
                        SetUINodeData(i, BaseShape);
                    }
                    if (ImGui::Selectable("Customize Base Shape", m_Ui.selectedNode.m_ID == MakeUINodeID(i, CustomizeBaseShape))) {
                        SetUINodeData(i, CustomizeBaseShape);
                    }
                    if (ImGui::Selectable("Base Noise", m_Ui.selectedNode.m_ID == MakeUINodeID(i, BaseNoise))) {
                        SetUINodeData(i, BaseNoise);
                    }
                    const bool filtersOpen = ImGui::TreeNodeEx("Filters", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
                    ImGui::SameLine();
                    const std::string addFilterPopupID = "Add Filter##" + biome->GetBiomeID().GetValue();
                    static char filterSearch[128]      = {};
                    if (ImGui::Button("Add##BiomeFilterAdd")) {
                        filterSearch[0] = '\0';
                        ImGui::OpenPopup(addFilterPopupID.c_str());
                    }
                    if (ImGui::BeginPopup(addFilterPopupID.c_str())) {
                        ImGui::SetNextItemWidth(240.0f);
                        ImGui::InputText("Search filters", filterSearch, IM_ARRAYSIZE(filterSearch));
                        ImGui::Separator();

                        std::map<std::string, std::vector<int>> visibleFiltersByCategory;
                        const auto &definitions = biome->GetFilterDefinitions();
                        for (int definitionIndex = 0; definitionIndex < static_cast<int>(definitions.size()); definitionIndex++) {
                            if (FuzzyFilterMatch(filterSearch, definitions[definitionIndex]->GetSearchText()))
                                visibleFiltersByCategory[definitions[definitionIndex]->GetCategory()].push_back(definitionIndex);
                        }

                        if (visibleFiltersByCategory.empty())
                            ImGui::TextDisabled("No filters match the search.");
                        for (const auto &[category, categoryDefinitions] : visibleFiltersByCategory) {
                            const std::string categoryLabel = category + "##FilterCategory";
                            const bool categoryOpen         = ImGui::TreeNodeEx(categoryLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
                            if (categoryOpen) {
                                for (const int definitionIndex : categoryDefinitions) {
                                    const auto &definition        = definitions[definitionIndex];
                                    const std::string filterLabel = definition->GetName() + "##" + definition->GetID();
                                    if (ImGui::Selectable(filterLabel.c_str())) {
                                        const int filterIndex = biome->AddFilter(definition);
                                        if (filterIndex >= 0) {
                                            m_Ui.selectedNode.m_BiomeIndex  = i;
                                            m_Ui.selectedNode.m_FilterIndex = filterIndex;
                                            m_Ui.selectedNode.m_BiomeID     = biome->GetBiomeID();
                                            m_Ui.selectedNode.m_ID          = biome->GetFilters()[filterIndex]->GetID();
                                            m_Ui.selectedNode.m_ObjectName  = SelectedUINodeObjectType_Filter;
                                        }
                                        ImGui::CloseCurrentPopup();
                                    }
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::EndPopup();
                    }
                    if (filtersOpen) {
                        const auto &filters = biome->GetFilters();
                        if (filters.size() == 0)
                            ImGui::Text("No Filters Added!");
                        bool filterWasRemoved = false;
                        if (filters.size() > 0 && ImGui::BeginTable("##FilterList", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
                            ImGui::TableSetupColumn("Filter", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());

                            for (int filterIndex = 0; filterIndex < static_cast<int>(filters.size()); filterIndex++) {
                                const auto &filter = filters[filterIndex];
                                ImGui::PushID(filter->GetID().c_str());
                                const bool selected = m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_Filter &&
                                                      m_Ui.selectedNode.m_BiomeIndex == i && m_Ui.selectedNode.m_FilterIndex == filterIndex;

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                if (ImGui::Selectable(filter->GetName().c_str(), selected)) {
                                    m_Ui.selectedNode.m_BiomeIndex  = i;
                                    m_Ui.selectedNode.m_FilterIndex = filterIndex;
                                    m_Ui.selectedNode.m_BiomeID     = biome->GetBiomeID();
                                    m_Ui.selectedNode.m_ID          = filter->GetID();
                                    m_Ui.selectedNode.m_ObjectName  = SelectedUINodeObjectType_Filter;
                                }

                                ImGui::TableSetColumnIndex(1);
                                const bool deleteFilter = ImGui::Button("X", ImVec2(-1.0f, 0.0f));
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Delete filter");
                                if (deleteFilter) {
                                    if (biome->RemoveFilter(filterIndex)) {
                                        if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_Filter &&
                                            m_Ui.selectedNode.m_BiomeIndex == i &&
                                            m_Ui.selectedNode.m_FilterIndex == filterIndex) {
                                            SetUINodeData(i, General);
                                            m_Ui.selectedNode.m_BiomeID = biome->GetBiomeID();
                                        } else if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_Filter &&
                                                   m_Ui.selectedNode.m_BiomeIndex == i &&
                                                   m_Ui.selectedNode.m_FilterIndex > filterIndex) {
                                            m_Ui.selectedNode.m_FilterIndex--;
                                        }
                                        filterWasRemoved = true;
                                    }
                                }
                                ImGui::PopID();
                                if (filterWasRemoved)
                                    break;
                            }
                            ImGui::EndTable();
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        ImGui::End();
    }

    void GenerationManager::ShowSettingsDetailed()
    {
        ImGui::Begin("Generation Settings", &m_AppState->windows.generationManager);

        if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_GlobalOptions) {
            ShowSettingsGlobalOptions();
        } else if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_GlobalBiomeMixer) {
            if (m_Field.biomeMixer->ShowSettings(m_Field.biomeManagers))
                m_AppState->generationDirtyManager.MarkMixer();
        } else {
            const int biomeIndex = m_Ui.selectedNode.m_BiomeIndex;
            if (biomeIndex >= 0 && biomeIndex < static_cast<int>(m_Field.biomeManagers.size())) {
                const auto &biome = m_Field.biomeManagers[static_cast<size_t>(biomeIndex)];
                if (biome != nullptr) {
                    switch (m_Ui.selectedNode.m_ObjectName) {
                        case SelectedUINodeObjectType_General:
                            biome->ShowGeneralSettings();
                            break;
                        case SelectedUINodeObjectType_BaseShape:
                            biome->ShowBaseShapeSettings();
                            break;
                        case SelectedUINodeObjectType_CustomizeBaseShape:
                            biome->ShowCustomizeBaseShapeSettings();
                            break;
                        case SelectedUINodeObjectType_BaseNoise:
                            biome->ShowBaseNoiseSettings();
                            break;
                        case SelectedUINodeObjectType_MaskTool:
                            biome->ShowMaskToolSettings();
                            break;
                        case SelectedUINodeObjectType_Filter:
                            biome->ShowFilterSettings(m_Ui.selectedNode.m_FilterIndex);
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        ImGui::End();
    }

    void GenerationManager::ShowSettingsGlobalOptions()
    {
        int storageMode             = m_Ui.fieldStorageUiMode;
        const char *storageLabels[] = {"R32F (32-bit float)", "R16F (16-bit float)"};
        if (ImGui::Combo("Field Storage", &storageMode, storageLabels, IM_ARRAYSIZE(storageLabels))) {
            m_Ui.fieldStorageUiMode         = storageMode;
            m_Ui.fieldStorageRestartPending = true;
            if (m_AppState->configManager != nullptr)
                m_AppState->configManager->SetString("generation", "field_storage", storageMode == 1 ? "R16F" : "R32F");
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Controls the precision of generated field textures.");
        if (m_Ui.fieldStorageRestartPending) {
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Restart required to apply storage change.");
        }
        ShowFieldStatistics();

        ImGui::Checkbox("Use Seed Texture", &m_Ui.useSeedFromActiveMesh);
        ImGui::Checkbox("Auto Updation Paused", &m_Ui.updationPaused);

        if (m_Ui.useSeedFromActiveMesh && m_Field.seedTexture == nullptr) {
            m_Field.seedTexture = std::make_shared<GeneratorTexture>(m_Ui.seedTextureResolution, m_Ui.seedTextureResolution);
            m_AppState->generationDirtyManager.MarkAllBiomes();
        } else if (!m_Ui.useSeedFromActiveMesh && m_Field.seedTexture != nullptr) {
            m_Field.seedTexture = nullptr;
            m_AppState->generationDirtyManager.MarkAllBiomes();
        }

        if (m_Ui.useSeedFromActiveMesh) {
            if (ImGui::CollapsingHeader("Seed Texture Settings")) {
                ImGui::BeginChild("Seed Texture Settings", ImVec2(0, 250), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
                ImGui::PushID("Seed Texture Settings");
                if (ImGui::Button("Pull From Active Mesh")) {
                    PullSeedTextureFromActiveMesh();
                    m_AppState->generationDirtyManager.MarkAllBiomes();
                }
                if (PowerOfTwoDropDown("Resolution", &m_Ui.seedTextureResolution, 2, 20)) {
                    m_Field.seedTexture->Resize(m_Ui.seedTextureResolution, m_Ui.seedTextureResolution);
                    m_AppState->generationDirtyManager.MarkAllBiomes();
                }
                ImGui::Image(m_Field.seedTexture->GetTextureID(), ImVec2(200, 200));
                ImGui::PopID();
                ImGui::EndChild();
            }
        }
    }

    void GenerationManager::UpdateFieldStatistics()
    {
        if (m_Field.statistics == nullptr || m_Field.heightmapData == nullptr)
            return;
        TF3D_PROFILE_SCOPE_DOMAIN("generation/statistics", PerformanceMonitor::Domain::Generation);
        {
            TF3D_PROFILE_SCOPE_DOMAIN("generation/statistics/dispatch", PerformanceMonitor::Domain::Generation);
            TF3D_PROFILE_GPU_SCOPE("generation/statistics/gpu");
            m_Field.statistics->Compute(m_Field.heightmapData.get(),
                                        m_AppState->mainMap.tileResolution,
                                        m_Field.statisticsSampleStride,
                                        true,
                                        -1.0f,
                                        m_AppState->constants.gpuWorkgroupSize);
        }
        {
            TF3D_PROFILE_SCOPE_DOMAIN("generation/statistics/finish", PerformanceMonitor::Domain::Wait);
            glFinish();
        }
        {
            TF3D_PROFILE_SCOPE_DOMAIN("generation/statistics/readback", PerformanceMonitor::Domain::Wait);
            m_Field.statisticsResult = m_Field.statistics->Read();
        }
    }

    bool GenerationManager::CommitHeightfield(uint64_t inputRevision)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/commit", PerformanceMonitor::Domain::Generation);
        if (!m_AppState->generationDirtyManager.ConsumeIfRevision(inputRevision)) {
            return false;
        }

        m_Field.heightmapData.swap(m_Field.workingHeightmapData);
        m_Field.slopeGenerator.swap(m_Field.workingSlopeGenerator);
        const uint64_t terrainRevision = m_TerrainRevision.fetch_add(1, std::memory_order_release) + 1;
        TF3D_PROFILE_SET_METADATA("terrain/revision", std::to_string(terrainRevision));
        TF3D_PROFILE_SET_METADATA("terrain/tile-resolution",
                                  std::to_string(m_AppState->mainMap.tileResolution));
        GenerateHeightmapMipmaps();
        if (m_Field.heightPyramid != nullptr) {
            m_Field.heightPyramid->Rebuild(m_Field.heightmapData.get());
        }
        for (const auto &biome : m_Field.biomeManagers) {
            if (biome != nullptr) {
                const auto state = biome->CaptureSnapshot().state;
                biome->MarkProcessed(state.revision);
            }
        }
        UpdateFieldStatistics();
        return true;
    }

    void GenerationManager::GenerateHeightmapMipmaps()
    {
        if (m_Field.heightmapData == nullptr || m_Field.heightmapData->GetResolution() <= 0)
            return;

        TF3D_PROFILE_SCOPE_DOMAIN("generation/heightfield-mipmap", PerformanceMonitor::Domain::Generation);
        TF3D_PROFILE_GPU_SCOPE("generation/heightfield-mipmap/gpu");
        TF3D_PROFILE_COUNTER_DOMAIN("gpu/mipmap-generations", 1.0, PerformanceMonitor::Domain::Gpu);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        m_Field.heightmapData->BindAsTexture(0);
        int32_t mipLevels = 1;
        for (int32_t mipSize = m_Field.heightmapData->GetResolution(); mipSize > 1; mipSize >>= 1)
            ++mipLevels;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipLevels - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GenerationManager::ShowFieldStatistics()
    {
        ImGui::Separator();
        ImGui::TextUnformatted("Final Field Statistics");
        if (!m_Field.statisticsResult.valid) {
            ImGui::TextDisabled("No completed statistics yet.");
            return;
        }

        ImGui::Text("Minimum: %.6f", m_Field.statisticsResult.minimum);
        ImGui::Text("Maximum: %.6f", m_Field.statisticsResult.maximum);
        ImGui::TextDisabled("Histogram sampled every %d pixels", m_Field.statisticsSampleStride);
        ImGui::PlotLines("##FinalFieldHistogram", m_Field.statisticsResult.histogram.data(),
                         static_cast<int>(m_Field.statisticsResult.histogram.size()), 0, "Height distribution", 0.0f, 1.0f,
                         ImVec2(-1.0f, 120.0f));
    }

    bool GenerationManager::OnTileResolutionChange(const std::string, void *)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/resize", PerformanceMonitor::Domain::Generation);
        WaitForGenerationWorker();
        m_Worker->ConsumeCompleted();
        TakeCompletedGenerationResult();
        TF3D_PROFILE_VALUE_DOMAIN("generation/resolution", m_AppState->mainMap.tileResolution, 0, 0,
                                  PerformanceMonitor::Domain::Generation);
        TF3D_PROFILE_SET_METADATA("terrain/tile-resolution",
                                  std::to_string(m_AppState->mainMap.tileResolution));
        auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
        m_Field.heightmapData->Resize(size);
        m_Field.workingHeightmapData->Resize(size);
        m_Field.swapBuffer->Resize(size);
        m_Field.slopeGenerator->Resize(m_AppState->mainMap.tileResolution);
        m_Field.workingSlopeGenerator->Resize(m_AppState->mainMap.tileResolution);
        m_AppState->generationDirtyManager.MarkForce(GenerationDirtyCause::Resize);
        for (auto biome : m_Field.biomeManagers) {
            biome->Resize();
        }

        m_ResolutionGenerationPending.store(true, std::memory_order_release);
        return false;
    }

} // namespace tf3d::generators
