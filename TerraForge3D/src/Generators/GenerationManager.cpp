#include "Generators/GenerationManager.h"
#include "Base/Shader.h"
#include "Data/ApplicationState.h"
#include "Data/ConfigManager.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"
#include <algorithm>
#include <exception>
#include <map>

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

        m_Worker = std::make_unique<GenerationWorker>(
            "Generation Worker", [this](uint64_t requestId) { ExecuteActiveGeneration(requestId); });

        m_DirtyManager.MarkForce(GenerationDirtyCause::Force);
    }

    void GenerationManager::ExecuteActiveGeneration(uint64_t requestId)
    {
        if (m_ActiveGeneration != nullptr) {
            try {
                m_ActiveGeneration->result = ExecuteGeneration(m_ActiveGeneration->snapshot, requestId);
            } catch (const std::exception &exception) {
                TF3D_LOG_ERROR("Generation request {} failed: {}", requestId, exception.what());
                m_ActiveGeneration->result = {
                    requestId, m_ActiveGeneration->snapshot.dirtyState.revision, false};
            } catch (...) {
                TF3D_LOG_ERROR("Generation request {} failed with an unknown exception", requestId);
                m_ActiveGeneration->result = {
                    requestId, m_ActiveGeneration->snapshot.dirtyState.revision, false};
            }
        } else {
            TF3D_LOG_ERROR("Generation worker completed request {} without an active job", requestId);
        }
    }

    void GenerationManager::Update()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/update", PerformanceMonitor::Domain::Generation);
        if (m_Worker->HasContext()) {
            m_Worker->Poll();
            if (m_Worker->IsCompleted()) {
                const uint64_t requestId  = m_Worker->GetCompletedRequestId();
                const bool matchingResult = m_ActiveGeneration != nullptr &&
                                            m_ActiveGeneration->result.requestId == requestId;
                const bool current = matchingResult &&
                                     m_ActiveGeneration->result.producedOutput &&
                                     IsCurrentGeneration(m_ActiveGeneration->result);
                if (current) {
                    CommitHeightfield(m_ActiveGeneration->snapshot);
                }
                if (matchingResult && !m_ActiveGeneration->result.producedOutput &&
                    !m_ActiveGeneration->result.superseded) {
                    const auto dirtyState = m_DirtyManager.Snapshot();
                    if (dirtyState.revision == m_ActiveGeneration->result.inputRevision) {
                        m_LastFailedGenerationRevision = dirtyState.revision;
                    }
                }
                m_ActiveGeneration.reset();
                m_Worker->ConsumeCompleted();
            }
        }
    }

    void GenerationManager::SchedulePendingGeneration()
    {
        const bool resolutionGenerationPending =
            m_ResolutionGenerationPending.load(std::memory_order_acquire);
        if (m_Ui.updationPaused && !resolutionGenerationPending) {
            return;
        }

        if ((!m_DirtyManager.IsDirty() && !resolutionGenerationPending) ||
            m_ActiveGeneration != nullptr ||
            m_DirtyManager.Snapshot().revision == m_LastFailedGenerationRevision ||
            !m_Worker->CanAcceptRequest()) {
            return;
        }

        m_ResolutionGenerationPending.store(false, std::memory_order_release);
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
        auto activeGeneration      = std::make_unique<ActiveGeneration>();
        activeGeneration->snapshot = CaptureGenerationSnapshot();
        m_ActiveGeneration         = std::move(activeGeneration);
        const auto &snapshot       = m_ActiveGeneration->snapshot;
        const auto request         = m_Worker->Request();
        uint64_t requestId         = request.requestId;
        if (request.status == GenerationWorker::RequestStatus::Unavailable) {
            requestId         = NextUniqueId();
            const auto result = ExecuteGeneration(snapshot, requestId);
            if (result.producedOutput && IsCurrentGeneration(result)) {
                CommitHeightfield(snapshot);
            }
            if (!result.producedOutput && !result.superseded) {
                const auto dirtyState = m_DirtyManager.Snapshot();
                if (dirtyState.revision == result.inputRevision) {
                    m_LastFailedGenerationRevision = dirtyState.revision;
                }
            }
            m_ActiveGeneration.reset();
        } else if (request.status == GenerationWorker::RequestStatus::Busy) {
            m_ActiveGeneration.reset();
        }
    }

    GenerationRequestSnapshot GenerationManager::CaptureGenerationSnapshot()
    {
        GenerationRequestSnapshot snapshot;
        snapshot.tileResolution = m_AppState->mainMap.tileResolution;
        snapshot.tileSize       = m_AppState->mainMap.tileSize;
        snapshot.dirtyState     = m_DirtyManager.Snapshot();
        CaptureGenerationState(snapshot);
        return snapshot;
    }

    void GenerationManager::CaptureGenerationState(GenerationRequestSnapshot &snapshot) const
    {
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
    }

    void GenerationManager::WaitForGenerationWorker()
    {
        m_Worker->WaitForIdle();
    }

    GenerationExecutionResult GenerationManager::ExecuteGeneration(const GenerationRequestSnapshot &snapshot,
                                                                   uint64_t requestId)
    {
        GenerationExecutionResult result{requestId, snapshot.dirtyState.revision, false};
        const auto &dirtyState = snapshot.dirtyState;
        TF3D_PROFILE_SCOPE_DOMAIN("generation/execute", PerformanceMonitor::Domain::Generation);

        const bool forceUpdate     = dirtyState.RequiresForce();
        const bool updateAllBiomes = dirtyState.Has(GenerationDirtyScope::AllBiomes);
        const GenerationContext context{snapshot.seedTexture.get(), snapshot.tileResolution,
                                        snapshot.gpuWorkgroupSize, snapshot.tileSize,
                                        &m_DirtyManager, dirtyState.revision};
        const auto isCurrent = [&result, &context] {
            const bool current = context.IsCurrent();
            result.superseded  = result.superseded || !current;
            return current;
        };
        if (!isCurrent()) {
            return result;
        }
        const size_t biomeCount = snapshot.biomes.size();
        bool biomeWorkRequested = false;
        bool biomeUpdateFailed  = false;
        for (size_t biomeIndex = 0; biomeIndex < biomeCount; ++biomeIndex) {
            if (!isCurrent()) {
                return result;
            }
            const auto &biomeSnapshot = snapshot.biomes[biomeIndex];
            const auto &biomeState    = biomeSnapshot.state;
            if (biomeSnapshot.runtime.data != nullptr &&
                (biomeState.updateRequired || updateAllBiomes || forceUpdate)) {
                biomeWorkRequested = true;
                if (!BiomeManager::Execute(&biomeSnapshot,
                                           &context,
                                           snapshot.swapBuffer.get()) &&
                    biomeState.enabled) {
                    biomeUpdateFailed = true;
                }
                if (!isCurrent()) {
                    return result;
                }
            }
        }
        if (biomeWorkRequested || dirtyState.Has(GenerationDirtyScope::Mixer) || forceUpdate) {
            if (biomeUpdateFailed || !isCurrent()) {
                return result;
            }
            bool producedOutput = false;
            producedOutput      = BiomeMixer::Execute(&snapshot.mixer,
                                                      &snapshot.mixerRuntime,
                                                      &context,
                                                      snapshot.biomes,
                                                      snapshot.workingHeightmapData.get(),
                                                      snapshot.swapBuffer.get());
            if (!isCurrent()) {
                return result;
            }
            if (producedOutput) {
                producedOutput = snapshot.slopeGenerator != nullptr &&
                                 snapshot.slopeGenerator->Compute(snapshot.workingHeightmapData.get(), &context);
            }
            if (!isCurrent()) {
                return result;
            }
            if (producedOutput && snapshot.statistics != nullptr) {
                TF3D_PROFILE_SCOPE_CHILD("statistics");
                TF3D_PROFILE_GPU_SCOPE_CHILD("gpu");
                snapshot.statistics->Compute(snapshot.workingHeightmapData.get(),
                                             snapshot.tileResolution,
                                             snapshot.statisticsSampleStride,
                                             true,
                                             -1.0f,
                                             snapshot.gpuWorkgroupSize);
                if (!isCurrent()) {
                    return result;
                }
            }
            if (!isCurrent()) {
                return result;
            }
            if (producedOutput) {
                GenerateHeightmapMipmaps(snapshot.workingHeightmapData.get());
            }
            result.producedOutput = producedOutput;
            return result;
        }
        return result;
    }

    bool GenerationManager::IsCurrentGeneration(const GenerationExecutionResult &result) const
    {
        if (!result.producedOutput || result.inputRevision == 0 || m_AppState == nullptr) {
            return false;
        }

        const auto dirtyState = m_DirtyManager.Snapshot();
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
            const std::string biomeName = "Biome " + std::to_string(m_Field.biomeManagers.size() + 1);
            m_Field.biomeManagers.push_back(
                std::make_shared<BiomeManager>(m_AppState, &m_DirtyManager, biomeName));
            m_DirtyManager.MarkMixer();
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
                    m_DirtyManager.MarkBiomes();
                    m_DirtyManager.MarkMixer();
                    m_Ui.selectedNode =
                        SelectedUINode(-1, SelectedUINodeObjectType_None, "None");
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
                    if (ImGui::Selectable(
                            "General", m_Ui.selectedNode.m_ID == SelectedUINode::MakeBiomeNodeID(i, "General"))) {
                        m_Ui.selectedNode =
                            SelectedUINode(i, SelectedUINodeObjectType_General, "General");
                    }
                    if (ImGui::Selectable(
                            "Mask", m_Ui.selectedNode.m_ID == SelectedUINode::MakeBiomeNodeID(i, "MaskTool"))) {
                        m_Ui.selectedNode =
                            SelectedUINode(i, SelectedUINodeObjectType_MaskTool, "MaskTool");
                    }
                    if (ImGui::Selectable(
                            "Base Shape", m_Ui.selectedNode.m_ID == SelectedUINode::MakeBiomeNodeID(i, "BaseShape"))) {
                        m_Ui.selectedNode =
                            SelectedUINode(i, SelectedUINodeObjectType_BaseShape, "BaseShape");
                    }
                    if (ImGui::Selectable("Customize Base Shape",
                                          m_Ui.selectedNode.m_ID ==
                                              SelectedUINode::MakeBiomeNodeID(i, "CustomizeBaseShape"))) {
                        m_Ui.selectedNode = SelectedUINode(
                            i, SelectedUINodeObjectType_CustomizeBaseShape, "CustomizeBaseShape");
                    }
                    if (ImGui::Selectable(
                            "Base Noise", m_Ui.selectedNode.m_ID == SelectedUINode::MakeBiomeNodeID(i, "BaseNoise"))) {
                        m_Ui.selectedNode =
                            SelectedUINode(i, SelectedUINodeObjectType_BaseNoise, "BaseNoise");
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
                                            m_Ui.selectedNode =
                                                SelectedUINode(i, SelectedUINodeObjectType_General, "General");
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
                m_DirtyManager.MarkMixer();
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
            m_DirtyManager.MarkAllBiomes();
        } else if (!m_Ui.useSeedFromActiveMesh && m_Field.seedTexture != nullptr) {
            m_Field.seedTexture = nullptr;
            m_DirtyManager.MarkAllBiomes();
        }

        if (m_Ui.useSeedFromActiveMesh) {
            if (ImGui::CollapsingHeader("Seed Texture Settings")) {
                ImGui::BeginChild("Seed Texture Settings", ImVec2(0, 250), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
                ImGui::PushID("Seed Texture Settings");
                if (ImGui::Button("Pull From Active Mesh")) {
                    PullSeedTextureFromActiveMesh();
                    m_DirtyManager.MarkAllBiomes();
                }
                if (PowerOfTwoDropDown("Resolution", &m_Ui.seedTextureResolution, 2, 20)) {
                    m_Field.seedTexture->Resize(m_Ui.seedTextureResolution, m_Ui.seedTextureResolution);
                    m_DirtyManager.MarkAllBiomes();
                }
                ImGui::Image(m_Field.seedTexture->GetTextureID(), ImVec2(200, 200));
                ImGui::PopID();
                ImGui::EndChild();
            }
        }
    }

    bool GenerationManager::CommitHeightfield(const GenerationRequestSnapshot &snapshot)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("generation/commit", PerformanceMonitor::Domain::Generation);
        if (!m_DirtyManager.ConsumeIfRevision(snapshot.dirtyState.revision)) {
            return false;
        }

        m_Field.heightmapData.swap(m_Field.workingHeightmapData);
        m_Field.slopeGenerator.swap(m_Field.workingSlopeGenerator);
        m_TerrainRevision.fetch_add(1, std::memory_order_release);
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

    void GenerationManager::GenerateHeightmapMipmaps(GeneratorData *heightmap)
    {
        if (heightmap == nullptr || heightmap->GetResolution() <= 0)
            return;

        TF3D_PROFILE_SCOPE_DOMAIN("generation/heightfield-mipmap", PerformanceMonitor::Domain::Generation);
        TF3D_PROFILE_GPU_SCOPE("generation/heightfield-mipmap/gpu");
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        heightmap->BindAsTexture(0);
        int32_t mipLevels = 1;
        for (int32_t mipSize = heightmap->GetResolution(); mipSize > 1; mipSize >>= 1)
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
        m_ActiveGeneration.reset();
        m_Worker->ConsumeCompleted();
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

        m_ResolutionGenerationPending.store(true, std::memory_order_release);
        return false;
    }

} // namespace tf3d::generators
