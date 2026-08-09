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

    bool BiomeManager::AddBaseShapeGenerator(const nlohmann::json &config, const std::string &source, const std::string &shaderPath)
    {
        auto generator = std::make_shared<BiomeBaseShapeGenerator>(m_AppState);
        if (!generator->LoadConfig(config, source, shaderPath))
            return false;
        m_BaseShapeGenerators.push_back(std::move(generator));
        return true;
    }

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
                if (!AddBaseShapeGenerator(config, source, relativeShaderPath))
                    TF3D_LOG_ERROR("Failed to load base-shape generator '{}'.", id);
            }
        }

        m_BaseNoiseGenerator = std::make_shared<BaseNoiseGenerator>(m_AppState);
        if (!m_BaseNoiseGenerator->Initialize())
            TF3D_LOG_ERROR("Failed to initialize base-noise generator.");

        m_DEMBaseShapeGenerator   = std::make_shared<DEMBaseShapeGenerator>(m_AppState);
        m_CalculatedMaskGenerator = std::make_shared<CalculatedMaskGenerator>(m_AppState);
        m_CustomizeBaseShape      = std::make_shared<BiomeCustomizeBaseShape>(m_AppState);
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
        m_CustomizeBaseShape->Resize();
        m_BaseNoiseGenerator->Resize(m_AppState->mainMap.tileResolution);
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

        if (m_SelectedBaseShapeGeneratorMode == BiomeBaseShapeGeneratorMode_Algorithm) {
            m_BaseShapeGenerators[m_SelectedBaseShapeGenerator]->Update(m_Data.get(), seedTexture, profilePrefix);
        } else if (m_SelectedBaseShapeGeneratorMode == BiomeBaseShapeGeneratorMode_GlobalElevation) {
            m_DEMBaseShapeGenerator->Update(m_Data.get(), seedTexture, profilePrefix);
        }

        if (m_CustomizeBaseShape->IsEnabled())
            m_CustomizeBaseShape->Update(m_Data.get(), swapBuffer, profilePrefix);
        else
            m_Data->CopyTo(swapBuffer);

        m_BaseNoiseGenerator->Update(swapBuffer, m_Data.get(), seedTexture, profilePrefix);
        m_FilterStack->Update(m_Data.get(), profilePrefix);
        m_CalculatedMaskGenerator->Invalidate();
        m_CalculatedMaskGenerator->Update(m_Data.get());

        m_RequireUpdation = false;
        m_StatisticsDirty = true;
    }

    bool BiomeManager::ShowCustomizeBaseShapeSettings()
    {
        ImGui::PushID(m_BiomeID.data());
        BIOME_UI_PROPERTY(m_CustomizeBaseShape->ShowSettings());
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
