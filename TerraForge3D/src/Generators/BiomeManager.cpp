#include "Generators/BiomeManager.h"
#include "Base/Base.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <filesystem>

#include "Generators/BiomeBaseShapeGenerator.h"

namespace tf3d::generators
{

    bool BiomeManager::AddBaseShapeGenerator(const std::string &config)
    {
        m_BaseShapeGenerators.push_back(std::make_shared<BiomeBaseShapeGenerator>(m_AppState));
        return m_BaseShapeGenerators.back()->LoadConfig(config);
    }

    bool BiomeManager::AddBaseShapeGenerator(const nlohmann::json &config, const std::string &source, const std::string &shaderPath)
    {
        m_BaseShapeGenerators.push_back(std::make_shared<BiomeBaseShapeGenerator>(m_AppState));
        return m_BaseShapeGenerators.back()->LoadConfig(config, source, shaderPath);
    }

    bool BiomeManager::LoadUpResources()
    {
        const std::string baseShapeGeneratorsDir = m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape";
        std::vector<std::filesystem::path> definitionPaths;
        std::vector<std::filesystem::path> legacyShaderPaths;
        for (const auto &directoryEntry : std::filesystem::recursive_directory_iterator(baseShapeGeneratorsDir)) {
            if (directoryEntry.is_directory())
                continue;
            if (directoryEntry.path().filename() == "base_shape.json") {
                definitionPaths.push_back(directoryEntry.path());
            } else if (directoryEntry.path().parent_path() == std::filesystem::path(baseShapeGeneratorsDir) &&
                       directoryEntry.path().extension() == ".glsl") {
                legacyShaderPaths.push_back(directoryEntry.path());
            }
        }
        std::sort(definitionPaths.begin(), definitionPaths.end());
        std::sort(legacyShaderPaths.begin(), legacyShaderPaths.end());
        for (const auto &definitionPath : definitionPaths) {
            bool loaded                    = false;
            const std::string configSource = ReadShaderSourceFile(definitionPath.string(), &loaded);
            if (!loaded)
                continue;
            try {
                const auto config            = nlohmann::json::parse(configSource);
                const std::string shaderName = config.value("Shader", "shape.glsl");
                const auto shaderPath        = definitionPath.parent_path() / shaderName;
                const std::string source     = ReadShaderSourceFile(shaderPath.string(), &loaded);
                if (!loaded) {
                    TF3D_LOG_ERROR("Failed to load base-shape shader '{}'.", shaderPath.string());
                    continue;
                }
                const auto relativeShaderPath = std::filesystem::relative(shaderPath, m_AppState->constants.shadersDir).generic_string();
                if (!AddBaseShapeGenerator(config, source, relativeShaderPath))
                    TF3D_LOG_ERROR("Failed to load base-shape generator '{}'.", definitionPath.string());
            } catch (const nlohmann::json::exception &exception) {
                TF3D_LOG_ERROR("Failed to parse base-shape metadata '{}': {}", definitionPath.string(), exception.what());
            }
        }
        for (const auto &shaderPath : legacyShaderPaths) {
            const std::string config = ReadShaderSourceFile(shaderPath.string(), &s_TempBool);
            if (!s_TempBool)
                continue;
            if (!AddBaseShapeGenerator(config))
                TF3D_LOG_ERROR("Failed to load base-shape generator '{}'.", shaderPath.string());
        }

        m_BaseNoiseGenerator           = std::make_shared<BiomeBaseNoiseGenerator>(m_AppState);
        const auto baseNoiseMetadata   = std::filesystem::path(m_AppState->constants.dataDir) / "inspectors" / "BaseNoise.json";
        const auto baseNoiseShaderPath = std::filesystem::path(m_AppState->constants.shadersDir) / "generation" / "base_noise" / "noise_gen.glsl";
        bool baseNoiseMetadataLoaded   = false;
        const auto baseNoiseConfigText = ReadShaderSourceFile(baseNoiseMetadata.string(), &baseNoiseMetadataLoaded);
        if (!baseNoiseMetadataLoaded) {
            TF3D_LOG_ERROR("Failed to load base-noise metadata '{}'.", baseNoiseMetadata.string());
        } else {
            try {
                const auto config          = nlohmann::json::parse(baseNoiseConfigText);
                bool baseNoiseShaderLoaded = false;
                const auto shaderSource    = m_AppState->resourceManager->LoadShaderSource("generation/base_noise/noise_gen", false, &baseNoiseShaderLoaded);
                if (!baseNoiseShaderLoaded) {
                    TF3D_LOG_ERROR("Failed to load base-noise shader '{}'.", baseNoiseShaderPath.string());
                } else {
                    const auto relativeShaderPath = std::filesystem::relative(baseNoiseShaderPath, m_AppState->constants.shadersDir).generic_string();
                    if (!m_BaseNoiseGenerator->LoadConfig(config, shaderSource, relativeShaderPath))
                        TF3D_LOG_ERROR("Failed to load base-noise generator '{}'.", baseNoiseMetadata.string());
                }
            } catch (const nlohmann::json::exception &exception) {
                TF3D_LOG_ERROR("Failed to parse base-noise metadata '{}': {}", baseNoiseMetadata.string(), exception.what());
            }
        }

        m_DEMBaseShapeGenerator   = std::make_shared<DEMBaseShapeGenerator>(m_AppState);
        m_CalculatedMaskGenerator = std::make_shared<CalculatedMaskGenerator>(m_AppState);
        m_CustomBaseShape         = std::make_shared<BiomeCustomBaseShape>(m_AppState);
        m_MaskTool                = std::make_shared<MaskTool>(m_AppState, glm::vec3(m_Color.x, m_Color.y, m_Color.z));
        m_FilterStack             = std::make_shared<BiomeFilterStack>(m_AppState);
        m_Statistics              = std::make_shared<GeneratorDataStatistics>(m_AppState);
        return true;
    }

    BiomeManager::BiomeManager(ApplicationState *appState)
    {
        m_AppState           = appState;
        m_BiomeID            = GenerateId(8);
        m_Color              = ImVec4((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1.0f);
        m_Data               = std::make_shared<GeneratorData>();
        static int s_BiomeID = 1;
        snprintf(m_BiomeName, 64, "Biome %d", s_BiomeID++);
        LoadUpResources();
        // m_SelectedBaseShapeGeneratorMode = BiomeBaseShapeGeneratorMode_GlobalElevation;
        for (auto i = 0; i < static_cast<int32_t>(m_BaseShapeGenerators.size()); i++) {
            if (m_BaseShapeGenerators[i]->GetName() == "Classic") {
                m_SelectedBaseShapeGenerator = i;
                break;
            }
        }

        this->Resize();
    }

    BiomeManager::~BiomeManager()
    {
    }

    void BiomeManager::Resize()
    {
        auto size = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileResolution * sizeof(float);
        m_Data->Resize(size);
        m_CustomBaseShape->Resize();
        m_CalculatedMaskGenerator->Resize(m_AppState->mainMap.tileResolution);
        m_MaskTool->Resize(m_AppState->mainMap.tileResolution);
        m_FilterStack->Resize(size, m_AppState->mainMap.tileResolution);
        m_RequireUpdation = true;
        m_StatisticsDirty = true;
    }

    void BiomeManager::Update(GeneratorData *swapBuffer, GeneratorTexture *seedTexture)
    {
        if (!m_IsEnabled)
            return;
        const std::string profilePrefix = std::string("generation/biome/") + m_BiomeName;
        TF3D_PROFILE_SCOPE_LAZY_DOMAIN(profilePrefix, PerformanceMonitor::Domain::Generation);
        TF3D_PROFILE_VALUE_DOMAIN("generation/biome/enabled", m_IsEnabled ? 1 : 0, 0, 0,
                                  PerformanceMonitor::Domain::Generation);
        // m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->Update(m_Data, seedTexture);

        if (!m_CustomBaseShape->IsEnabled() || m_CustomBaseShape->RequiresBaseShapeUpdate()) {
            if (m_SelectedBaseShapeGeneratorMode == BiomeBaseShapeGeneratorMode_Algorithm) {
                m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->Update(m_Data.get(), seedTexture, profilePrefix);
            } else if (m_SelectedBaseShapeGeneratorMode == BiomeBaseShapeGeneratorMode_GlobalElevation) {
                m_DEMBaseShapeGenerator->Update(m_Data.get(), seedTexture, profilePrefix);
            }
        }

        if (m_CustomBaseShape->IsEnabled()) {
            m_CustomBaseShape->Update(m_Data.get(), m_Data.get(), swapBuffer, profilePrefix);
        }

        m_Data->CopyTo(swapBuffer); // temporary will later be
        // optimized when filters are implemented

        m_BaseNoiseGenerator->Update(swapBuffer, m_Data.get(), seedTexture, profilePrefix);
        m_FilterStack->Update(m_Data.get(), profilePrefix);
        m_CalculatedMaskGenerator->Invalidate();
        m_CalculatedMaskGenerator->Update(m_Data.get());

        m_RequireUpdation = false;
        m_StatisticsDirty = true;
    }

    bool BiomeManager::ShowCustomBaseShapeSettings()
    {
        ImGui::PushID(m_BiomeID.data());
        BIOME_UI_PROPERTY(m_CustomBaseShape->ShowShettings());
        ImGui::PopID();
        return m_RequireUpdation;
    }

    bool BiomeManager::ShowBaseShapeSettings()
    {
        ImGui::PushID(m_BiomeID.data());

        if (ImGui::BeginCombo("Base Shape Generator Mode", s_BaseShapeGeneratorModeNames[m_SelectedBaseShapeGeneratorMode].c_str())) {
            for (int i = 0; i < static_cast<int32_t>(s_BaseShapeGeneratorModeNames.size()); i++) {
                bool isSelected = m_SelectedBaseShapeGeneratorMode == i;
                if (ImGui::Selectable(s_BaseShapeGeneratorModeNames[i].c_str(), isSelected)) {
                    m_SelectedBaseShapeGeneratorMode = static_cast<BiomeBaseShapeGeneratorMode>(i);
                    m_RequireUpdation                = true;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (m_SelectedBaseShapeGeneratorMode == BiomeBaseShapeGeneratorMode_Algorithm) {
            if (ImGui::BeginCombo("Style", m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->GetName().c_str())) {
                for (int i = 0; i < static_cast<int32_t>(m_BaseShapeGenerators.size()); i++) {
                    bool isSelected = m_SelectedBaseShapeGenerator == i;
                    if (ImGui::Selectable(m_BaseShapeGenerators[i]->GetName().c_str(), isSelected)) {
                        m_SelectedBaseShapeGenerator = i;
                        m_RequireUpdation            = true;
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            BIOME_UI_PROPERTY(m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->ShowSettings());
        } else if (m_SelectedBaseShapeGeneratorMode == BiomeBaseShapeGeneratorMode_GlobalElevation) {
            BIOME_UI_PROPERTY(m_DEMBaseShapeGenerator->ShowSettings());
        }

        ImGui::PopID();
        return m_RequireUpdation;
    }

    bool BiomeManager::ShowGeneralSettings()
    {
        ImGui::PushID(m_BiomeID.data());
        ImGui::InputText("Biome Name", m_BiomeName, sizeof(m_BiomeName));
        BIOME_UI_PROPERTY(ImGui::Checkbox("Enabled", &m_IsEnabled));
        if (ImGui::ColorEdit3("Biome Color", reinterpret_cast<float *>(&m_Color))) {
            m_MaskTool->SetVizColor(m_Color.x, m_Color.y, m_Color.z);
        }

        if (ImGui::CollapsingHeader("Statistics")) {
            if (m_StatisticsDirty && m_Statistics != nullptr && m_Data != nullptr) {
                TF3D_PROFILE_SCOPE_DOMAIN("generation/biome/statistics", PerformanceMonitor::Domain::Generation);
                {
                    TF3D_PROFILE_GPU_SCOPE("generation/biome/statistics/gpu");
                    m_Statistics->Compute(m_Data.get(), m_AppState->mainMap.tileResolution, m_StatisticsSampleStride);
                }
                {
                    TF3D_PROFILE_SCOPE_DOMAIN("generation/biome/statistics/finish", PerformanceMonitor::Domain::Wait);
                    glFinish();
                }
                {
                    TF3D_PROFILE_SCOPE_DOMAIN("generation/biome/statistics/readback", PerformanceMonitor::Domain::Wait);
                    m_StatisticsResult = m_Statistics->Read();
                }
                m_StatisticsDirty = false;
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
        return m_RequireUpdation;
    }

    bool BiomeManager::ShowMaskToolSettings()
    {
        ImGui::PushID(m_BiomeID.data());
        if (m_MaskTool->IsShowingGeneratedMask() && ImGui::CollapsingHeader("Generated mask source", ImGuiTreeNodeFlags_DefaultOpen)) {
            BIOME_UI_PROPERTY(m_CalculatedMaskGenerator->ShowSettings());
        }
        m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator->GetTexture(), "Calculated terrain mask");
        BIOME_UI_PROPERTY(m_MaskTool->ShowSettings());
        ImGui::PopID();
        return m_RequireUpdation;
    }

    int BiomeManager::AddFilter(const std::shared_ptr<BiomeFilterDefinition> &definition)
    {
        const int index = m_FilterStack->AddFilter(definition);
        if (index >= 0)
            m_RequireUpdation = true;
        return index;
    }

    bool BiomeManager::RemoveFilter(int filterIndex)
    {
        if (m_FilterStack == nullptr || !m_FilterStack->RemoveFilter(filterIndex))
            return false;
        m_RequireUpdation = true;
        return true;
    }

    bool BiomeManager::ShowFilterSettings(int filterIndex)
    {
        BIOME_UI_PROPERTY(m_FilterStack->ShowSettings(filterIndex));
        return m_RequireUpdation;
    }

    bool BiomeManager::ShowBaseNoiseSettings()
    {
        ImGui::PushID(m_BiomeID.data());
        BIOME_UI_PROPERTY(m_BaseNoiseGenerator->ShowSettings());
        ImGui::PopID();
        return m_RequireUpdation;
    }

} // namespace tf3d::generators
