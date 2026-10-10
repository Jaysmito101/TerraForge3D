#include "Generators/GenerationManager.h"

#include "Base/Logging/Logger.h"
#include "Base/Mesh.h"
#include "Base/Model.h"
#include "Data/ApplicationState.h"
#include "Data/ConfigManager.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace tf3d::generators
{
    namespace
    {

        constexpr int kFilterSearchBufferSize = 128;

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

            if (ImGui::Button("Delete")) {
                TF3D_LOG_DEBUG("Loaded {} biome managers", context.field.biomeManagers.size());
                context.field.biomeManagers.erase(context.field.biomeManagers.begin() + biomeIndex);
                TF3D_LOG_DEBUG("Active biome managers: {}", context.field.biomeManagers.size());
                context.dirtyManager.MarkBiomes();
                context.dirtyManager.MarkMixer();
                context.ui.selectedNode = SelectedUINode(-1, SelectedUINodeObjectType_None, "None");

                if (biomeOpen) {
                    ImGui::TreePop();
                }
                ImGui::PopID();
                return false;
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
                context.field.biomeManagers.push_back(
                    std::make_shared<BiomeManager>(&context.appState, &context.dirtyManager, biomeName));
                context.dirtyManager.MarkMixer();
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
                context.ui.fieldStorageUiMode         = storageMode;
                context.ui.fieldStorageRestartPending = true;
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
        if (!m_Ui.useSeedFromActiveMesh) {
            return;
        }

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
