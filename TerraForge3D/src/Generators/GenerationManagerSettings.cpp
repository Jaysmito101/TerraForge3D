#include "Generators/GenerationManager.h"

#include "Base/Logging/Logger.h"
#include "Base/Mesh.h"
#include "Base/Model.h"
#include "Data/ApplicationState.h"
#include "Data/ConfigManager.h"
#include "Exporters/Serializer.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <vector>

namespace tf3d::generators
{
    namespace
    {

        constexpr int kFilterSearchBufferSize       = 128;
        constexpr int32_t kMinimumApiTileResolution = 16;
        constexpr int32_t kMaximumApiTileResolution = 1 << 20;
        constexpr int32_t kMinimumApiSeedResolution = 4;
        constexpr int32_t kMaximumApiSeedResolution = 1 << 20;
        constexpr int32_t kMaximumTileCount         = 1000000;

        bool IsPowerOfTwoInRange(int32_t value, int32_t minimum, int32_t maximum)
        {
            return value >= minimum && value <= maximum && (value & (value - 1)) == 0;
        }

        int32_t GetMaximumTextureSize()
        {
            GLint maximumTextureSize = 0;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
            return maximumTextureSize;
        }

        void ClearSeedTexture(GeneratorTexture &texture)
        {
            texture.MakeCPUCopy();
            texture.ZeroCPUCopy();
            texture.UploadCPUCopy();
            texture.FreeCPUCopy();
        }

        struct SettingsViewContext {
            ApplicationState &appState;
            GenerationDirtyManager &dirtyManager;
            FieldState &field;
            UiState &ui;
        };

        void ShowGlobalInspectorNodes(UiState &ui)
        {
            if (ImGui::Selectable("Options", ui.selectedNode.m_ID == "GlobalOptions")) {
                ui.selectedNode.m_ID         = "GlobalOptions";
                ui.selectedNode.m_ObjectName = SelectedUINodeObjectType_GlobalOptions;
            }

            if (ImGui::Selectable("Biome Mixer", ui.selectedNode.m_ID == "GlobalBiomeMixer")) {
                ui.selectedNode.m_ID         = "GlobalBiomeMixer";
                ui.selectedNode.m_ObjectName = SelectedUINodeObjectType_GlobalBiomeMixer;
            }
        }

        void SelectBiomeNode(UiState &ui,
                             int biomeIndex,
                             const char *label,
                             const char *nodeName,
                             SelectedUINodeObjectType objectType)
        {
            const std::string nodeId = SelectedUINode::MakeBiomeNodeID(biomeIndex, nodeName);
            if (ImGui::Selectable(label, ui.selectedNode.m_ID == nodeId)) {
                ui.selectedNode = SelectedUINode(biomeIndex, objectType, nodeName);
            }
        }

        void ShowBiomeNodeSelection(UiState &ui, int biomeIndex)
        {
            SelectBiomeNode(ui, biomeIndex, "General", "General", SelectedUINodeObjectType_General);
            SelectBiomeNode(ui, biomeIndex, "Mask", "MaskTool", SelectedUINodeObjectType_MaskTool);
            SelectBiomeNode(ui, biomeIndex, "Base Shape", "BaseShape", SelectedUINodeObjectType_BaseShape);
            SelectBiomeNode(ui, biomeIndex, "Customize Base Shape", "CustomizeBaseShape",
                            SelectedUINodeObjectType_CustomizeBaseShape);
            SelectBiomeNode(ui, biomeIndex, "Base Noise", "BaseNoise", SelectedUINodeObjectType_BaseNoise);
        }

        void SelectFilterNode(UiState &ui,
                              const BiomeManager &biome,
                              int biomeIndex,
                              int filterIndex)
        {
            const auto &filter            = biome.GetFilters()[static_cast<std::size_t>(filterIndex)];
            ui.selectedNode.m_BiomeIndex  = biomeIndex;
            ui.selectedNode.m_FilterIndex = filterIndex;
            ui.selectedNode.m_BiomeID     = biome.GetBiomeID();
            ui.selectedNode.m_ID          = filter->GetID();
            ui.selectedNode.m_ObjectName  = SelectedUINodeObjectType_Filter;
        }

        void ShowAddFilterPopup(UiState &ui,
                                BiomeManager &biome,
                                int biomeIndex,
                                const std::string &popupId,
                                char *filterSearch)
        {
            if (!ImGui::BeginPopup(popupId.c_str())) {
                return;
            }

            ImGui::SetNextItemWidth(240.0f);
            ImGui::InputText("Search filters", filterSearch, kFilterSearchBufferSize);
            ImGui::Separator();

            std::map<std::string, std::vector<int>> definitionsByCategory;
            const auto &definitions = biome.GetFilterDefinitions();
            for (int definitionIndex = 0; definitionIndex < static_cast<int>(definitions.size()); ++definitionIndex) {
                if (FuzzyFilterMatch(filterSearch, definitions[definitionIndex]->GetSearchText())) {
                    definitionsByCategory[definitions[definitionIndex]->GetCategory()].push_back(definitionIndex);
                }
            }

            if (definitionsByCategory.empty()) {
                ImGui::TextDisabled("No filters match the search.");
            }

            for (const auto &[category, categoryDefinitions] : definitionsByCategory) {
                const std::string categoryLabel = category + "##FilterCategory";
                const bool categoryOpen         = ImGui::TreeNodeEx(
                    categoryLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
                if (!categoryOpen) {
                    continue;
                }

                for (const int definitionIndex : categoryDefinitions) {
                    const auto &definition        = definitions[static_cast<std::size_t>(definitionIndex)];
                    const std::string filterLabel = definition->GetName() + "##" + definition->GetID();
                    if (ImGui::Selectable(filterLabel.c_str())) {
                        const int filterIndex = biome.AddFilter(definition);
                        if (filterIndex >= 0) {
                            SelectFilterNode(ui, biome, biomeIndex, filterIndex);
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::TreePop();
            }

            ImGui::EndPopup();
        }

        void RepairFilterSelectionAfterRemoval(UiState &ui,
                                               const BiomeManager &biome,
                                               int biomeIndex,
                                               int removedFilterIndex)
        {
            const bool selectedBiomeFilter = ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_Filter &&
                                             ui.selectedNode.m_BiomeIndex == biomeIndex;
            if (!selectedBiomeFilter) {
                return;
            }

            if (ui.selectedNode.m_FilterIndex == removedFilterIndex) {
                ui.selectedNode           = SelectedUINode(biomeIndex, SelectedUINodeObjectType_General, "General");
                ui.selectedNode.m_BiomeID = biome.GetBiomeID();
            } else if (ui.selectedNode.m_FilterIndex > removedFilterIndex) {
                --ui.selectedNode.m_FilterIndex;
            }
        }

        void ShowBiomeFilterList(UiState &ui, BiomeManager &biome, int biomeIndex)
        {
            const auto &filters = biome.GetFilters();
            if (filters.empty()) {
                ImGui::Text("No Filters Added!");
            }

            if (filters.empty() ||
                !ImGui::BeginTable("##FilterList", 2,
                                   ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
                return;
            }

            ImGui::TableSetupColumn("Filter", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());

            bool filterWasRemoved = false;
            for (int filterIndex = 0; filterIndex < static_cast<int>(filters.size()); ++filterIndex) {
                const auto &filter = filters[static_cast<std::size_t>(filterIndex)];
                ImGui::PushID(filter->GetID().c_str());

                const bool selected = ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_Filter &&
                                      ui.selectedNode.m_BiomeIndex == biomeIndex &&
                                      ui.selectedNode.m_FilterIndex == filterIndex;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (ImGui::Selectable(filter->GetName().c_str(), selected)) {
                    SelectFilterNode(ui, biome, biomeIndex, filterIndex);
                }

                ImGui::TableSetColumnIndex(1);
                const bool deleteFilter = ImGui::Button("X", ImVec2(-1.0f, 0.0f));
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Delete filter");
                }

                if (deleteFilter && biome.RemoveFilter(filterIndex)) {
                    RepairFilterSelectionAfterRemoval(ui, biome, biomeIndex, filterIndex);
                    filterWasRemoved = true;
                }

                ImGui::PopID();
                if (filterWasRemoved) {
                    break;
                }
            }

            ImGui::EndTable();
        }

        void ShowBiomeFilterTree(SettingsViewContext &context,
                                 BiomeManager &biome,
                                 int biomeIndex)
        {
            const bool filtersOpen = ImGui::TreeNodeEx(
                "Filters", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
            ImGui::SameLine();

            const std::string popupId                         = "Add Filter##" + biome.GetBiomeID().GetValue();
            static char filterSearch[kFilterSearchBufferSize] = {};
            if (ImGui::Button("Add##BiomeFilterAdd")) {
                filterSearch[0] = '\0';
                ImGui::OpenPopup(popupId.c_str());
            }
            ShowAddFilterPopup(context.ui, biome, biomeIndex, popupId, filterSearch);

            if (!filtersOpen) {
                return;
            }

            ShowBiomeFilterList(context.ui, biome, biomeIndex);
            ImGui::TreePop();
        }

        bool ShowBiomeInspectorEntry(SettingsViewContext &context, int biomeIndex)
        {
            const auto biome = context.field.biomeManagers[static_cast<std::size_t>(biomeIndex)];
            ImGui::PushID(biome->GetBiomeID().c_str());

            const bool biomeOpen = ImGui::TreeNodeEx(
                biome->GetBiomeName(), ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
            ImGui::SameLine();

            const bool canDeleteBiome = context.field.biomeManagers.size() > 1;
            ImGui::BeginDisabled(!canDeleteBiome);
            const bool deleteBiome = ImGui::Button("Delete");
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered() && !canDeleteBiome) {
                ImGui::SetTooltip("At least one biome is required by the generation graph.");
            }

            if (deleteBiome && context.appState.generationManager != nullptr) {
                std::string error;
                if (context.appState.generationManager->RemoveBiome(biome->GetBiomeID().GetValue(), error)) {
                    context.ui.selectedNode = SelectedUINode(-1, SelectedUINodeObjectType_None, "None");

                    if (biomeOpen) {
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                    return false;
                } else {
                    TF3D_LOG_WARN("Could not remove biome: {}", error);
                }
            }

            if (biomeOpen) {
                ShowBiomeNodeSelection(context.ui, biomeIndex);
                ShowBiomeFilterTree(context, *biome, biomeIndex);
                ImGui::TreePop();
            }

            ImGui::PopID();
            return true;
        }

        void ShowBiomeInspectorList(SettingsViewContext &context)
        {
            const bool biomesOpen = ImGui::TreeNodeEx(
                "Biomes", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
            ImGui::SameLine();

            if (ImGui::Button("Add##BiomeAdd")) {
                const std::string biomeName = "Biome " + std::to_string(context.field.biomeManagers.size() + 1);
                if (context.appState.generationManager != nullptr) {
                    std::string biomeId;
                    std::string error;
                    if (!context.appState.generationManager->CreateBiome(biomeName, biomeId, error)) {
                        TF3D_LOG_WARN("Could not add biome: {}", error);
                    }
                }
            }

            if (!biomesOpen) {
                return;
            }

            if (context.field.biomeManagers.empty()) {
                ImGui::Text("No Biomes Added!");
            }

            for (int biomeIndex = 0; biomeIndex < static_cast<int>(context.field.biomeManagers.size()); ++biomeIndex) {
                if (!ShowBiomeInspectorEntry(context, biomeIndex)) {
                    break;
                }
            }

            ImGui::TreePop();
        }

        void ShowSelectedBiomeSettings(SettingsViewContext &context)
        {
            const int biomeIndex = context.ui.selectedNode.m_BiomeIndex;
            if (biomeIndex < 0 || biomeIndex >= static_cast<int>(context.field.biomeManagers.size())) {
                return;
            }

            const auto &biome = context.field.biomeManagers[static_cast<std::size_t>(biomeIndex)];
            if (biome == nullptr) {
                return;
            }

            switch (context.ui.selectedNode.m_ObjectName) {
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
                    biome->ShowFilterSettings(context.ui.selectedNode.m_FilterIndex);
                    break;
                default:
                    break;
            }
        }

        void ShowSelectedSettings(SettingsViewContext &context)
        {
            if (context.ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_GlobalBiomeMixer) {
                if (context.field.biomeMixer->ShowSettings(context.field.biomeManagers)) {
                    context.dirtyManager.MarkMixer();
                }
                return;
            }

            ShowSelectedBiomeSettings(context);
        }

        void ShowFieldStorageOption(SettingsViewContext &context)
        {
            int storageMode             = context.ui.fieldStorageUiMode;
            const char *storageLabels[] = {"R32F (32-bit float)", "R16F (16-bit float)"};
            if (ImGui::Combo("Field Storage", &storageMode, storageLabels, IM_ARRAYSIZE(storageLabels))) {
                context.ui.fieldStorageUiMode = storageMode;
                const auto requestedStorage   = storageMode == 1 ? GeneratorDataStorage::R16F
                                                                 : GeneratorDataStorage::R32F;
                context.ui.fieldStorageRestartPending =
                    requestedStorage != context.field.heightmapData->GetStorage();
                if (context.appState.configManager != nullptr) {
                    context.appState.configManager->SetString(
                        "generation", "field_storage", storageMode == 1 ? "R16F" : "R32F");
                }
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Controls the precision of generated field textures.");
            }

            if (context.ui.fieldStorageRestartPending) {
                ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Restart required to apply storage change.");
            }
        }

        void UpdateSeedTextureState(SettingsViewContext &context)
        {
            if (context.ui.useSeedFromActiveMesh && context.field.seedTexture == nullptr) {
                context.field.seedTexture = std::make_shared<GeneratorTexture>(
                    context.ui.seedTextureResolution, context.ui.seedTextureResolution);
                context.dirtyManager.MarkAllBiomes();
            } else if (!context.ui.useSeedFromActiveMesh && context.field.seedTexture != nullptr) {
                context.field.seedTexture = nullptr;
                context.dirtyManager.MarkAllBiomes();
            }
        }

    } // namespace

    exporters::SerializerNode GenerationManager::SaveGlobalSettings() const
    {
        auto settings = exporters::CreateSerializerNode();
        auto tile     = exporters::CreateSerializerNode();
        tile->Set("resolution", m_AppState != nullptr ? m_AppState->mainMap.tileResolution : 0);
        tile->Set("size", m_AppState != nullptr ? m_AppState->mainMap.tileSize : 0.0f);
        settings->Set("tile", tile);
        settings->Set("autoUpdatePaused", m_Ui.updationPaused);

        auto seedTexture = exporters::CreateSerializerNode();
        seedTexture->Set("enabled", m_Ui.useSeedFromActiveMesh);
        seedTexture->Set("resolution", m_Ui.seedTextureResolution);
        seedTexture->Set("source", m_Ui.seedTextureSource);
        settings->Set("seedTexture", seedTexture);

        const auto requestedStorage = m_Ui.fieldStorageUiMode == 1
                                          ? GeneratorDataStorage::R16F
                                          : GeneratorDataStorage::R32F;
        settings->Set("fieldStorage", requestedStorage);
        return settings;
    }

    exporters::SerializerNode GenerationManager::GetGlobalSettingsStatus() const
    {
        auto status             = exporters::CreateSerializerNode();
        auto tile               = exporters::CreateSerializerNode();
        const int32_t tileCount = m_AppState != nullptr ? m_AppState->mainMap.tileCount : 0;
        tile->Set("count", tileCount);
        tile->Set("mapResolution", m_AppState != nullptr ? m_AppState->mainMap.mapResolution : 0);
        status->Set("tile", tile);

        auto limits = exporters::CreateSerializerNode();
        limits->Set("maximumTextureSize", GetMaximumTextureSize());
        status->Set("limits", limits);

        const auto dirtyState = m_DirtyManager.Snapshot();
        auto dirty            = exporters::CreateSerializerNode();
        dirty->Set("isDirty", dirtyState.IsDirty());
        dirty->Set("revision", std::to_string(dirtyState.revision));
        dirty->Set("scopeMask", dirtyState.mask);
        dirty->Set("causeMask", dirtyState.causeMask);
        status->Set("dirty", dirty);

        const auto activeStorage    = m_Field.heightmapData != nullptr
                                          ? m_Field.heightmapData->GetStorage()
                                          : GeneratorData::GetDefaultStorage();
        const auto requestedStorage = m_Ui.fieldStorageUiMode == 1
                                          ? GeneratorDataStorage::R16F
                                          : GeneratorDataStorage::R32F;
        auto fieldStorage           = exporters::CreateSerializerNode();
        fieldStorage->Set("active", activeStorage);
        fieldStorage->Set("restartRequired", activeStorage != requestedStorage);
        status->Set("fieldStorage", fieldStorage);
        return status;
    }

    GenerationGlobalSettingsApplyResult GenerationManager::LoadGlobalSettings(
        const exporters::SerializerNode &settings,
        std::string &error)
    {
        error.clear();

        if (settings == nullptr) {
            error = "Generation settings payload is unavailable.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }

        const bool hasTileSettings = settings->HasKey("tile");
        const auto tileSettings    = settings->Get<exporters::SerializerNode>("tile");
        const bool hasSeedSettings = settings->HasKey("seedTexture");
        const auto seedSettings    = settings->Get<exporters::SerializerNode>("seedTexture");

        if ((hasTileSettings && tileSettings == nullptr) || (hasSeedSettings && seedSettings == nullptr)) {
            error = "Tile and seedTexture settings must be objects.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }

        const bool hasTileResolution = tileSettings != nullptr && tileSettings->HasKey("resolution");
        const bool hasTileSize       = tileSettings != nullptr && tileSettings->HasKey("size");
        const bool hasUpdatesPaused  = settings->HasKey("autoUpdatePaused");
        const bool hasSeedEnabled    = seedSettings != nullptr && seedSettings->HasKey("enabled");
        const bool hasSeedResolution = seedSettings != nullptr && seedSettings->HasKey("resolution");
        const bool hasSeedSource     = seedSettings != nullptr && seedSettings->HasKey("source");
        const bool hasFieldStorage   = settings->HasKey("fieldStorage");
        const bool hasUpdate         = hasTileResolution || hasTileSize || hasUpdatesPaused || hasSeedEnabled ||
                               hasSeedResolution || hasSeedSource || hasFieldStorage;
        if (!hasUpdate) {
            error = "At least one generation setting must be provided.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }

        if (m_AppState == nullptr) {
            error = "Generation settings are unavailable.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }

        const int32_t nextTileResolution = hasTileResolution
                                               ? tileSettings->Get<int32_t>("resolution")
                                               : m_AppState->mainMap.tileResolution;
        if (hasTileResolution &&
            !IsPowerOfTwoInRange(nextTileResolution, kMinimumApiTileResolution, kMaximumApiTileResolution)) {
            error = "tileResolution must be a power of two from 16 through 1048576.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }
        if (hasTileResolution) {
            const int32_t maximumTextureSize = GetMaximumTextureSize();
            if (maximumTextureSize <= 0 || nextTileResolution > maximumTextureSize) {
                error = "tileResolution exceeds the active GPU's maximum 2D texture size.";
                return GenerationGlobalSettingsApplyResult::Invalid;
            }
        }

        int32_t nextTileCount = m_AppState->mainMap.tileCount;
        if (hasTileSize) {
            const float requestedSize = tileSettings->Get<float>("size");
            if (!std::isfinite(requestedSize) || requestedSize <= 0.0f || requestedSize > 1.0f) {
                error = "tileSize must be finite and between 1 / 1000000 and 1.";
                return GenerationGlobalSettingsApplyResult::Invalid;
            }

            const double requestedCount = 1.0 / static_cast<double>(requestedSize);
            if (!std::isfinite(requestedCount) || requestedCount < 1.0 ||
                requestedCount > static_cast<double>(kMaximumTileCount)) {
                error = "tileSize must correspond to a tile count from 1 through 1000000.";
                return GenerationGlobalSettingsApplyResult::Invalid;
            }

            nextTileCount              = static_cast<int32_t>(std::llround(requestedCount));
            const float normalizedSize = 1.0f / static_cast<float>(nextTileCount);
            const float sizeTolerance  = std::max(1.0e-8f, normalizedSize * 1.0e-6f);
            if (std::abs(requestedSize - normalizedSize) > sizeTolerance) {
                error = "tileSize must be the reciprocal of a whole tile count.";
                return GenerationGlobalSettingsApplyResult::Invalid;
            }
        }

        const int64_t nextMapResolution = static_cast<int64_t>(nextTileResolution) * nextTileCount;
        if (nextMapResolution > std::numeric_limits<int32_t>::max()) {
            error = "tileResolution multiplied by tile count exceeds the supported map resolution.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }

        const int32_t nextSeedResolution = hasSeedResolution
                                               ? seedSettings->Get<int32_t>("resolution")
                                               : m_Ui.seedTextureResolution;
        if (hasSeedResolution &&
            !IsPowerOfTwoInRange(nextSeedResolution, kMinimumApiSeedResolution, kMaximumApiSeedResolution)) {
            error = "seedTextureResolution must be a power of two from 4 through 1048576.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }
        if (hasSeedResolution) {
            const int32_t maximumTextureSize = GetMaximumTextureSize();
            if (maximumTextureSize <= 0 || nextSeedResolution > maximumTextureSize) {
                error = "seedTextureResolution exceeds the active GPU's maximum 2D texture size.";
                return GenerationGlobalSettingsApplyResult::Invalid;
            }
        }

        const bool nextSeedEnabled = hasSeedEnabled
                                         ? seedSettings->Get<bool>("enabled")
                                         : m_Ui.useSeedFromActiveMesh;
        const auto nextSeedSource  = hasSeedSource
                                         ? seedSettings->Get<GenerationSeedTextureSource>("source")
                                         : m_Ui.seedTextureSource;
        if (nextSeedSource != GenerationSeedTextureSource::None &&
            nextSeedSource != GenerationSeedTextureSource::MainModel) {
            error = "seedTexture.source is not a supported source.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }

        const auto nextFieldStorage = hasFieldStorage
                                          ? settings->Get<GeneratorDataStorage>("fieldStorage")
                                          : GeneratorDataStorage::R32F;
        if (hasFieldStorage && nextFieldStorage != GeneratorDataStorage::R16F &&
            nextFieldStorage != GeneratorDataStorage::R32F) {
            error = "fieldStorage is not a supported storage format.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }

        const bool seedResolutionChanged = nextSeedResolution != m_Ui.seedTextureResolution;
        const bool seedEnabledChanged    = nextSeedEnabled != m_Ui.useSeedFromActiveMesh;
        const bool seedSourceChanged     = nextSeedSource != m_Ui.seedTextureSource;
        const bool seedTextureChanged    = (nextSeedEnabled || m_Ui.useSeedFromActiveMesh) &&
                                        (seedResolutionChanged || seedEnabledChanged || seedSourceChanged);
        const bool needsMainModel = nextSeedEnabled && nextSeedSource == GenerationSeedTextureSource::MainModel &&
                                    (seedSourceChanged || seedEnabledChanged || seedResolutionChanged);
        if (needsMainModel &&
            (m_AppState->mainModel == nullptr || m_AppState->mainModel->mesh == nullptr)) {
            error = "The mainModel seed source is unavailable.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }

        const bool tileResolutionChanged = nextTileResolution != m_AppState->mainMap.tileResolution;
        const bool tileCountChanged      = nextTileCount != m_AppState->mainMap.tileCount;
        if ((tileResolutionChanged || tileCountChanged) && m_AppState->eventManager == nullptr) {
            error = "Tile settings cannot be applied because the application event manager is unavailable.";
            return GenerationGlobalSettingsApplyResult::Invalid;
        }

        if (hasFieldStorage) {
            if (m_AppState->configManager == nullptr) {
                error = "Field storage cannot be changed because application configuration is unavailable.";
                return GenerationGlobalSettingsApplyResult::PersistenceFailed;
            }
            const char *storageName = nextFieldStorage == GeneratorDataStorage::R16F ? "R16F" : "R32F";
            if (!m_AppState->configManager->SetString("generation", "field_storage", storageName)) {
                error = "The field storage selection could not be saved to application configuration.";
                return GenerationGlobalSettingsApplyResult::PersistenceFailed;
            }
        }

        if (hasUpdatesPaused) {
            m_Ui.updationPaused = settings->Get<bool>("autoUpdatePaused");
        }

        if (hasFieldStorage) {
            m_Ui.fieldStorageUiMode         = nextFieldStorage == GeneratorDataStorage::R16F ? 1 : 0;
            const auto activeStorage        = m_Field.heightmapData != nullptr
                                                  ? m_Field.heightmapData->GetStorage()
                                                  : GeneratorData::GetDefaultStorage();
            m_Ui.fieldStorageRestartPending = nextFieldStorage != activeStorage;
        }

        m_AppState->mainMap.tileResolution = nextTileResolution;
        m_AppState->mainMap.tileCount      = nextTileCount;
        m_AppState->mainMap.mapResolution  = static_cast<int32_t>(nextMapResolution);
        m_AppState->mainMap.tileSize       = 1.0f / static_cast<float>(nextTileCount);
        m_AppState->mainMap.currentTileX   = std::clamp(m_AppState->mainMap.currentTileX, 0, nextTileCount - 1);
        m_AppState->mainMap.currentTileY   = std::clamp(m_AppState->mainMap.currentTileY, 0, nextTileCount - 1);
        m_AppState->mainMap.tileOffsetX    = m_AppState->mainMap.currentTileX * m_AppState->mainMap.tileSize;
        m_AppState->mainMap.tileOffsetY    = m_AppState->mainMap.currentTileY * m_AppState->mainMap.tileSize;

        if (seedTextureChanged) {
            WaitForGenerationWorker();
            m_ActiveGeneration.reset();
            m_Worker->TryConsumeCompleted();
        }

        m_Ui.useSeedFromActiveMesh = nextSeedEnabled;
        m_Ui.seedTextureResolution = nextSeedResolution;
        m_Ui.seedTextureSource     = nextSeedSource;
        if (nextSeedEnabled) {
            if (m_Field.seedTexture == nullptr) {
                m_Field.seedTexture = std::make_shared<GeneratorTexture>(nextSeedResolution, nextSeedResolution);
            } else if (m_Field.seedTexture->GetWidth() != nextSeedResolution ||
                       m_Field.seedTexture->GetHeight() != nextSeedResolution) {
                m_Field.seedTexture->Resize(nextSeedResolution, nextSeedResolution);
            }

            if (nextSeedSource == GenerationSeedTextureSource::MainModel && needsMainModel) {
                PullSeedTextureFromActiveMesh();
            } else if (nextSeedSource == GenerationSeedTextureSource::None && seedSourceChanged) {
                ClearSeedTexture(*m_Field.seedTexture);
            }
        } else if (seedEnabledChanged) {
            m_Field.seedTexture.reset();
        }

        if (seedTextureChanged) {
            m_DirtyManager.MarkAllBiomes();
        }

        if (tileResolutionChanged) {
            m_AppState->eventManager->RaiseEvent("TileResolutionChanged", std::to_string(nextTileResolution));
        } else if (tileCountChanged) {
            m_DirtyManager.MarkForce(GenerationDirtyCause::External);
        }

        return GenerationGlobalSettingsApplyResult::Applied;
    }

    void GenerationManager::ShowSettings()
    {
        ShowSettingsInspector();
        ShowSettingsDetailed();
    }

    void GenerationManager::ShowSettingsInspector()
    {
        ImGui::Begin("Generator Inspector", &m_AppState->windows.generationManager);

        SettingsViewContext context{*m_AppState, m_DirtyManager, m_Field, m_Ui};
        ShowGlobalInspectorNodes(m_Ui);
        ShowBiomeInspectorList(context);

        ImGui::End();
    }

    void GenerationManager::ShowSettingsDetailed()
    {
        ImGui::Begin("Generation Settings", &m_AppState->windows.generationManager);

        if (!m_Ui.useWorkerThread) {
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
                               "Warning: inline generation is selected. New requests will run on the render thread and may pause the UI.");
            ImGui::Separator();
        } else if (!m_Worker->HasContext()) {
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
                               "Warning: the generation worker is unavailable. Generation is running inline on the render thread, so the UI may pause while it runs.");
            ImGui::Separator();
        }

        if (m_Ui.selectedNode.m_ObjectName == SelectedUINodeObjectType_GlobalOptions) {
            ShowSettingsGlobalOptions();
        } else {
            SettingsViewContext context{*m_AppState, m_DirtyManager, m_Field, m_Ui};
            ShowSelectedSettings(context);
        }

        ImGui::End();
    }

    void GenerationManager::ShowSettingsGlobalOptions()
    {
        SettingsViewContext context{*m_AppState, m_DirtyManager, m_Field, m_Ui};
        ShowFieldStorageOption(context);
        ShowFieldStatistics();

        ImGui::Checkbox("Use Seed Texture", &m_Ui.useSeedFromActiveMesh);
        ImGui::Checkbox("Auto Updation Paused", &m_Ui.updationPaused);

        if (ImGui::Checkbox("Run Generation on Worker Thread", &m_Ui.useWorkerThread) &&
            m_AppState->configManager != nullptr) {
            m_AppState->configManager->SetBool("generation", "use_worker_thread", m_Ui.useWorkerThread);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When disabled, generation runs inline on the render thread and may pause the UI.");
        }

        UpdateSeedTextureState(context);

        if (!m_Ui.useSeedFromActiveMesh || m_Field.seedTexture == nullptr ||
            !ImGui::CollapsingHeader("Seed Texture Settings")) {
            return;
        }

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
                         static_cast<int>(m_Field.statisticsResult.histogram.size()), 0, "Height distribution", 0.0f,
                         1.0f, ImVec2(-1.0f, 120.0f));
    }

    void GenerationManager::PullSeedTextureFromActiveMesh()
    {
        if (!m_Ui.useSeedFromActiveMesh || m_Field.seedTexture == nullptr || m_AppState == nullptr ||
            m_AppState->mainModel == nullptr || m_AppState->mainModel->mesh == nullptr) {
            return;
        }

        m_Ui.seedTextureSource = GenerationSeedTextureSource::MainModel;
        m_Field.seedTexture->MakeCPUCopy();
        m_Field.seedTexture->ZeroCPUCopy();
        const auto mesh = m_AppState->mainModel->mesh;
        for (int vertexIndex = 0; vertexIndex < mesh->GetVertexCount(); ++vertexIndex) {
            const auto &vertex = mesh->GetVertex(vertexIndex);
            m_Field.seedTexture->SetPixel(vertex.texCoord.x, vertex.texCoord.y,
                                          vertex.position.x, vertex.position.z, vertex.position.y);
        }
        m_Field.seedTexture->UploadCPUCopy();
        m_Field.seedTexture->FreeCPUCopy();
    }

} // namespace tf3d::generators
