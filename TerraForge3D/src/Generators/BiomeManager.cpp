#include "Generators/BiomeManager.h"
#include "Base/Base.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "Utils/JsonIncludeResolver.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <filesystem>
#include <unordered_set>

#include "Generators/BiomeBaseShapeGenerator.h"

namespace tf3d::generators
{

    namespace
    {
        bool ResolveBaseShapeShaderPath(const std::filesystem::path &shaderRoot,
                                        const std::string &relativePath,
                                        std::filesystem::path &resolvedPath,
                                        std::string *error)
        {
            auto fail = [&](const std::string &message) {
                if (error != nullptr)
                    *error = message;
                return false;
            };

            const std::filesystem::path relative(relativePath);
            if (relative.empty() || relative.is_absolute() || relative.extension() != ".glsl")
                return fail("shader path must be a relative .glsl path");

            std::error_code errorCode;
            const auto root = std::filesystem::weakly_canonical(shaderRoot, errorCode);
            if (errorCode)
                return fail("could not resolve shader root");

            errorCode.clear();
            const auto candidate = std::filesystem::weakly_canonical(root / relative, errorCode);
            if (errorCode || !std::filesystem::is_regular_file(candidate))
                return fail("shader file does not exist");

            const auto relativeCandidate   = candidate.lexically_relative(root);
            const std::string relativeText = relativeCandidate.generic_string();
            if (relativeCandidate.empty() || relativeText == ".." || relativeText.starts_with("../"))
                return fail("shader path escapes the shader root");

            resolvedPath = candidate;
            return true;
        }
    } // namespace

    bool BiomeManager::LoadUpResources()
    {
        const auto dataDirectory = std::filesystem::path(m_AppState->constants.dataDir);
        const auto inspectorPath = inspector::CustomInspector::GetConfigPath(dataDirectory, "BaseShape");
        utils::JsonIncludeResolverOptions resolverOptions;
        resolverOptions.rootDirectory  = dataDirectory / "inspectors";
        resolverOptions.pathMode       = utils::JsonIncludePathMode::RelativeToIncludingFile;
        resolverOptions.restrictToRoot = true;
        const utils::JsonIncludeResolver jsonResolver(resolverOptions);

        std::string resolveError;
        const auto catalog = jsonResolver.ResolveFile(inspectorPath, &resolveError);
        if (!catalog) {
            TF3D_LOG_ERROR("Failed to load base-shape inspector '{}': {}", inspectorPath.string(), resolveError);
        } else if (!catalog->is_object() || !catalog->contains("Sections") || !(*catalog)["Sections"].is_array()) {
            TF3D_LOG_ERROR("Base-shape inspector '{}' must contain a Sections array.", inspectorPath.string());
        } else {
            const auto shaderRoot = std::filesystem::path(m_AppState->constants.shadersDir);
            std::unordered_set<std::string> shapeIDs;
            for (size_t index = 0; index < (*catalog)["Sections"].size(); ++index) {
                const auto &shape = (*catalog)["Sections"][index];
                if (!shape.is_object()) {
                    TF3D_LOG_WARN("Skipping base-shape inspector section {}: expected an object.", index);
                    continue;
                }

                const auto customDataIterator = shape.find("CustomData");
                if (customDataIterator == shape.end() || !customDataIterator->is_object()) {
                    TF3D_LOG_ERROR("Skipping base-shape inspector section {}: CustomData must be an object.", index);
                    continue;
                }

                const auto &customData       = *customDataIterator;
                const std::string id         = customData.value("ID", "");
                const std::string shaderName = customData.value("Shader", "");
                if (!utils::IsPascalIdentifier(id) || !shapeIDs.insert(utils::CanonicalID(id)).second) {
                    TF3D_LOG_ERROR("Skipping base-shape inspector section {}: CustomData.ID must be a unique PascalCase identifier.", index);
                    continue;
                }
                if (!shape.contains("Sections") || !shape["Sections"].is_array()) {
                    TF3D_LOG_ERROR("Skipping base-shape '{}': Sections must be an array.", id);
                    continue;
                }

                std::filesystem::path shaderPath;
                std::string shaderPathError;
                if (!ResolveBaseShapeShaderPath(shaderRoot, shaderName, shaderPath, &shaderPathError)) {
                    TF3D_LOG_ERROR("Skipping base-shape '{}': {} ({})", id, shaderPathError, shaderName);
                    continue;
                }

                bool loaded              = false;
                const std::string source = ReadShaderSourceFile(shaderPath.string(), &loaded);
                if (!loaded) {
                    TF3D_LOG_ERROR("Skipping base-shape '{}': could not read shader '{}'.", id, shaderPath.string());
                    continue;
                }

                nlohmann::json config                = shape;
                config["ID"]                         = id;
                config["Name"]                       = shape.value("Name", id);
                config["Description"]                = shape.value("Description", "");
                const std::string relativeShaderPath = std::filesystem::path(shaderName).generic_string();

                auto generator = BiomeBaseShapeGenerator::Create(m_AppState, config, source, relativeShaderPath);
                if (generator) {
                    m_BaseShapeGenerators.push_back(std::move(generator));
                } else {
                    TF3D_LOG_ERROR("Failed to load base-shape generator '{}'.", id);
                }
            }
        }

        m_BaseNoiseGenerator = std::make_shared<BaseNoiseGenerator>(m_AppState);
        if (!m_BaseNoiseGenerator->Initialize())
            TF3D_LOG_ERROR("Failed to initialize base-noise generator.");

        m_DEMBaseShapeGenerator = std::make_shared<DEMBaseShapeGenerator>(m_AppState);
        m_CustomizeBaseShape    = std::make_shared<BiomeCustomizeBaseShape>(m_AppState);
        m_MaskLayer             = std::make_shared<MaskLayer>(m_AppState, glm::vec3(m_Color.x, m_Color.y, m_Color.z),
                                                              "None");
        m_FilterStack           = std::make_shared<BiomeFilterStack>(m_AppState);
        m_Statistics            = std::make_shared<GeneratorDataStatistics>(m_AppState);
        return true;
    }

    BiomeManager::BiomeManager(data::ApplicationState *appState)
    {
        m_AppState           = appState;
        m_BiomeID            = BiomeID{GenerateId(8)};
        m_Color              = ImVec4((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1.0f);
        m_Data               = std::make_shared<GeneratorData>();
        static int s_BiomeID = 1;
        snprintf(m_BiomeName, 64, "Biome %d", s_BiomeID++);
        LoadUpResources();
        // m_SelectedBaseShapeGeneratorMode.store(BiomeBaseShapeGeneratorMode_GlobalElevation);
        for (auto i = 0; i < static_cast<int32_t>(m_BaseShapeGenerators.size()); i++) {
            if (m_BaseShapeGenerators[i]->GetName() == "Classic") {
                m_SelectedBaseShapeGenerator.store(i, std::memory_order_release);
                break;
            }
        }

        this->Resize();
    }

    BiomeManager::~BiomeManager()
    {
    }

    void BiomeManager::MarkUpdateRequired()
    {
        m_UpdateTracker.Publish();
        if (m_AppState != nullptr) {
            m_AppState->generationDirtyManager.MarkBiomes();
        }
    }

    void BiomeManager::Resize()
    {
        auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
        m_Data->Resize(size);
        m_CustomizeBaseShape->Resize();
        m_BaseNoiseGenerator->Resize(m_AppState->mainMap.tileResolution);
        m_MaskLayer->Resize(m_AppState->mainMap.tileResolution);
        m_FilterStack->Resize(size, m_AppState->mainMap.tileResolution);
        MarkUpdateRequired();
        m_StatisticsDirty.store(true, std::memory_order_release);
    }

    BiomeManager::Snapshot BiomeManager::CaptureSnapshot() const
    {
        Snapshot snapshot;
        auto &state              = snapshot.state;
        state.revision           = m_UpdateTracker.PublishedRevision();
        state.updateRequired     = m_UpdateTracker.RequiresUpdate();
        state.enabled            = m_IsEnabled;
        state.baseShapeMode      = m_SelectedBaseShapeGeneratorMode.load(std::memory_order_acquire);
        state.baseShapeGenerator = m_SelectedBaseShapeGenerator.load(std::memory_order_acquire);
        if (m_MaskLayer != nullptr) {
            state.mask.emplace(m_MaskLayer->GetState());
        }

        if (state.baseShapeMode == BiomeBaseShapeGeneratorMode_Algorithm &&
            state.baseShapeGenerator >= 0 &&
            state.baseShapeGenerator < static_cast<int32_t>(m_BaseShapeGenerators.size())) {
            const auto &generator = m_BaseShapeGenerators[static_cast<size_t>(state.baseShapeGenerator)];
            if (generator != nullptr)
                state.baseShape.emplace(generator->GetState());
        } else if (state.baseShapeMode == BiomeBaseShapeGeneratorMode_GlobalElevation &&
                   m_DEMBaseShapeGenerator != nullptr) {
            state.demBaseShape.emplace(m_DEMBaseShapeGenerator->GetState());
        }
        if (m_BaseNoiseGenerator != nullptr) {
            state.baseNoise.emplace(m_BaseNoiseGenerator->GetState());
        }
        if (m_CustomizeBaseShape != nullptr) {
            state.customBaseShape.emplace(m_CustomizeBaseShape->GetState());
        }
        if (m_FilterStack != nullptr) {
            state.filters.emplace(m_FilterStack->GetState());
        }
        auto &runtime                 = snapshot.runtime;
        runtime.id                    = m_BiomeID;
        runtime.name                  = m_BiomeName;
        runtime.data                  = m_Data;
        runtime.maskTexture           = m_MaskLayer != nullptr ? m_MaskLayer->GetTextureHandle() : nullptr;
        runtime.baseShapeGenerators   = m_BaseShapeGenerators;
        runtime.demBaseShapeGenerator = m_DEMBaseShapeGenerator;
        runtime.baseNoiseGenerator    = m_BaseNoiseGenerator;
        runtime.customBaseShape       = m_CustomizeBaseShape;
        runtime.filterStack           = m_FilterStack;
        runtime.maskLayer             = m_MaskLayer;
        return snapshot;
    }

    bool BiomeManager::Execute(const Snapshot *snapshot,
                               const GenerationContext *context,
                               GeneratorData *swapBuffer)
    {
        if (snapshot == nullptr || snapshot->runtime.data == nullptr ||
            context == nullptr || swapBuffer == nullptr) {
            return false;
        }

        const auto &state   = snapshot->state;
        const auto &runtime = snapshot->runtime;
        if (!state.enabled) {
            return true;
        }

        auto *biomeData = runtime.data.get();

        TF3D_PROFILE_SCOPE_CHILD_LAZY(runtime.name);
        TF3D_PROFILE_VALUE_DOMAIN("generation/biome/enabled", state.enabled ? 1 : 0, 0, 0,
                                  PerformanceMonitor::Domain::Generation);

        if (state.baseShapeMode == BiomeBaseShapeGeneratorMode_Algorithm &&
            state.baseShapeGenerator >= 0 &&
            state.baseShapeGenerator < static_cast<int32_t>(runtime.baseShapeGenerators.size()) &&
            state.baseShape.has_value()) {

            const auto &generator = runtime.baseShapeGenerators[static_cast<size_t>(state.baseShapeGenerator)];
            if (generator != nullptr)
                generator->Update(&*state.baseShape, context, biomeData);

        } else if (state.baseShapeMode == BiomeBaseShapeGeneratorMode_GlobalElevation &&
                   runtime.demBaseShapeGenerator != nullptr && state.demBaseShape.has_value()) {
            runtime.demBaseShapeGenerator->Update(&*state.demBaseShape, context, biomeData);
        }

        if (runtime.customBaseShape != nullptr && state.customBaseShape.has_value()) {
            runtime.customBaseShape->Update(&*state.customBaseShape, context, biomeData);
        }
        biomeData->CopyTo(swapBuffer);

        if (runtime.baseNoiseGenerator != nullptr && state.baseNoise.has_value()) {
            runtime.baseNoiseGenerator->Update(&*state.baseNoise,
                                               context,
                                               swapBuffer,
                                               biomeData);
        } else {
            swapBuffer->CopyTo(biomeData);
        }
        if (runtime.filterStack != nullptr && state.filters.has_value()) {
            runtime.filterStack->Update(&*state.filters, context, biomeData);
        }
        if (runtime.maskLayer != nullptr && state.mask.has_value()) {
            runtime.maskLayer->Apply(state.mask->value, context, biomeData);
        }

        return true;
    }

    void BiomeManager::MarkProcessed(Revision revision)
    {
        m_UpdateTracker.MarkProcessed(revision);
        m_StatisticsDirty.store(true, std::memory_order_release);
    }

    void BiomeManager::ShowCustomizeBaseShapeSettings()
    {
        ImGui::PushID(m_BiomeID.c_str());
        const auto previousStateRevision = m_CustomizeBaseShape->GetStateRevision();
        m_CustomizeBaseShape->ShowSettings();
        if (m_CustomizeBaseShape->GetStateRevision() != previousStateRevision) {
            MarkUpdateRequired();
        }
        ImGui::PopID();
    }

    void BiomeManager::ShowBaseShapeSettings()
    {
        ImGui::PushID(m_BiomeID.c_str());

        const auto requestedMode = m_SelectedBaseShapeGeneratorMode.load(std::memory_order_acquire);
        const int modeIndex      = std::clamp(static_cast<int>(requestedMode), 0,
                                              static_cast<int>(s_BaseShapeGeneratorModeNames.size()) - 1);
        if (ImGui::BeginCombo("Base Shape Generator Mode", s_BaseShapeGeneratorModeNames[modeIndex].c_str())) {
            for (int i = 0; i < static_cast<int32_t>(s_BaseShapeGeneratorModeNames.size()); i++) {
                bool isSelected = modeIndex == i;
                if (ImGui::Selectable(s_BaseShapeGeneratorModeNames[i].c_str(), isSelected)) {
                    m_SelectedBaseShapeGeneratorMode.store(static_cast<BiomeBaseShapeGeneratorMode>(i),
                                                           std::memory_order_release);
                    MarkUpdateRequired();
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        const auto selectedMode = m_SelectedBaseShapeGeneratorMode.load(std::memory_order_acquire);
        const int selectedIndex = m_SelectedBaseShapeGenerator.load(std::memory_order_acquire);
        if (selectedMode == BiomeBaseShapeGeneratorMode_Algorithm &&
            selectedIndex >= 0 && selectedIndex < static_cast<int>(m_BaseShapeGenerators.size())) {
            const auto generatorIndex = static_cast<size_t>(selectedIndex);
            auto &generator           = m_BaseShapeGenerators[generatorIndex];
            if (ImGui::BeginCombo("Style", generator->GetName().c_str())) {
                for (int i = 0; i < static_cast<int32_t>(m_BaseShapeGenerators.size()); i++) {
                    bool isSelected = selectedIndex == i;
                    if (ImGui::Selectable(m_BaseShapeGenerators[i]->GetName().c_str(), isSelected)) {
                        m_SelectedBaseShapeGenerator.store(i, std::memory_order_release);
                        MarkUpdateRequired();
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            const auto previousRevision = generator->GetStateRevision();
            generator->ShowSettings();
            if (generator->GetStateRevision() != previousRevision) {
                MarkUpdateRequired();
            }
        } else if (selectedMode == BiomeBaseShapeGeneratorMode_GlobalElevation) {
            if (m_BaseNoiseGenerator != nullptr) {
                ImGui::Separator();
                ImGui::TextUnformatted("Base Noise");
                const auto previousRevision = m_BaseNoiseGenerator->GetStateRevision();
                m_BaseNoiseGenerator->ShowEnabledControl(true);
                if (m_BaseNoiseGenerator->GetStateRevision() != previousRevision) {
                    MarkUpdateRequired();
                }
            }
            if (m_DEMBaseShapeGenerator->ShowSettings()) {
                MarkUpdateRequired();
            }
        }

        ImGui::PopID();
    }

    void BiomeManager::ShowGeneralSettings()
    {
        ImGui::PushID(m_BiomeID.c_str());
        ImGui::InputText("Biome Name", m_BiomeName, sizeof(m_BiomeName));
        if (ImGui::Checkbox("Enabled", &m_IsEnabled))
            MarkUpdateRequired();
        if (ImGui::ColorEdit3("Biome Color", reinterpret_cast<float *>(&m_Color))) {
            m_MaskLayer->SetVizColor(m_Color.x, m_Color.y, m_Color.z);
        }

        if (ImGui::CollapsingHeader("Statistics")) {
            if (m_StatisticsDirty.load(std::memory_order_acquire) && m_Statistics != nullptr && m_Data != nullptr) {
                TF3D_PROFILE_SCOPE_DOMAIN("generation/biome/statistics", PerformanceMonitor::Domain::Generation);
                {
                    TF3D_PROFILE_GPU_SCOPE("generation/biome/statistics/gpu");
                    m_Statistics->Compute(m_Data.get(),
                                          m_AppState->mainMap.tileResolution,
                                          m_StatisticsSampleStride,
                                          true,
                                          -1.0f,
                                          m_AppState->constants.gpuWorkgroupSize);
                }
                {
                    TF3D_PROFILE_SCOPE_DOMAIN("generation/biome/statistics/finish", PerformanceMonitor::Domain::Wait);
                    glFinish();
                }
                {
                    TF3D_PROFILE_SCOPE_DOMAIN("generation/biome/statistics/readback", PerformanceMonitor::Domain::Wait);
                    m_StatisticsResult = m_Statistics->Read();
                }
                m_StatisticsDirty.store(false, std::memory_order_release);
            }

            if (!m_StatisticsResult.valid) {
                ImGui::TextDisabled("No completed statistics yet.");
            } else {
                ImGui::Text("Minimum: %.6f", m_StatisticsResult.minimum);
                ImGui::Text("Maximum: %.6f", m_StatisticsResult.maximum);
                ImGui::TextDisabled("Histogram sampled every %d pixels", m_StatisticsSampleStride);
                ImGui::PlotLines("##BiomeFieldHistogram", m_StatisticsResult.histogram.data(),
                                 static_cast<int>(m_StatisticsResult.histogram.size()), 0, "Height distribution", 0.0f, 1.0f,
                                 ImVec2(-1.0f, 120.0f));
            }
        }
        ImGui::PopID();
    }

    void BiomeManager::ShowMaskToolSettings()
    {
        ImGui::PushID(m_BiomeID.c_str());
        if (m_MaskLayer->ShowSettings())
            MarkUpdateRequired();
        ImGui::PopID();
    }

    int BiomeManager::AddFilter(const std::shared_ptr<BiomeFilterDefinition> &definition)
    {
        const int index = m_FilterStack->AddFilter(definition);
        if (index >= 0)
            MarkUpdateRequired();
        return index;
    }

    bool BiomeManager::RemoveFilter(int filterIndex)
    {
        if (m_FilterStack == nullptr || !m_FilterStack->RemoveFilter(filterIndex))
            return false;
        MarkUpdateRequired();
        return true;
    }

    void BiomeManager::ShowFilterSettings(int filterIndex)
    {
        if (m_FilterStack->ShowSettings(filterIndex))
            MarkUpdateRequired();
    }

    void BiomeManager::ShowBaseNoiseSettings()
    {
        ImGui::PushID(m_BiomeID.c_str());
        const auto previousRevision = m_BaseNoiseGenerator->GetStateRevision();
        m_BaseNoiseGenerator->ShowSettings();
        if (m_BaseNoiseGenerator->GetStateRevision() != previousRevision) {
            MarkUpdateRequired();
        }
        ImGui::PopID();
    }

} // namespace tf3d::generators
