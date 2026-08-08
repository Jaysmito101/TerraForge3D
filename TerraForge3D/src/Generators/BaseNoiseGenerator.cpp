#include "Generators/BaseNoiseGenerator.h"

#include "Data/ApplicationState.h"
#include "Generators/NoiseAlgorithmConfig.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

namespace tf3d::generators
{

    namespace
    {
        constexpr int kBaseNoiseOctaveCount = 10;

        void EnsureNoiseValues(CustomInspector &inspector, int defaultNoiseAlgorithm)
        {
            if (!inspector.Contains("NoiseAlgorithm"))
                inspector.Add("NoiseAlgorithm", defaultNoiseAlgorithm);
            if (!inspector.Contains("NoiseOctaves"))
                inspector.Add("NoiseOctaves", kBaseNoiseOctaveCount);
            if (!inspector.Contains("NoiseWarp"))
                inspector.Add("NoiseWarp", 0.0f);
            if (!inspector.Contains("NoiseJitter"))
                inspector.Add("NoiseJitter", 0.75f);
        }

        std::vector<float> DefaultNoiseOctaveStrengths()
        {
            std::vector<float> values(kBaseNoiseOctaveCount, 1.0f);
            values[0] = values[1] = 0.0f;
            return values;
        }

        std::vector<float> NormalizeNoiseOctaveStrengths(const std::vector<float> &source)
        {
            auto values = DefaultNoiseOctaveStrengths();
            for (size_t index = 0; index < source.size() && index < values.size(); ++index)
                values[index] = glm::clamp(source[index], 0.0f, 1.0f);
            return values;
        }

        void EnsureNoiseOctaveComponent(CustomInspector &inspector)
        {
            bool hasOctaveComponent = false;
            std::vector<std::string> legacyWidgetLabels;
            for (const auto &[label, widget] : inspector.GetWidgets()) {
                if (widget.GetVariableName() == "OctaveStrengths" && widget.GetType() == CustomInspectorWidgetType::Octaves)
                    hasOctaveComponent = true;
                if (widget.GetVariableName().rfind("NoiseOctaveStrength", 0) == 0)
                    legacyWidgetLabels.push_back(label);
            }
            for (const auto &label : legacyWidgetLabels)
                inspector.RemoveWidget(label);

            if (!hasOctaveComponent) {
                inspector.BeginSection("Octaves");
                auto &widget = inspector.AddWidget("Octave Strengths", CustomInspectorWidgetType::Octaves, "OctaveStrengths");
                widget.SetConstraints(0.0f, 1.0f);
                widget.SetShaderUniformName("u_NoiseOctaveStrengths");
                widget.SetTooltip("Controls the contribution of each individual noise layer.");
                inspector.EndSection();
            }
        }
    } // namespace

    BaseNoiseGenerator::BaseNoiseGenerator(ApplicationState *appState)
        : m_AppState(appState), m_Inspector(std::make_shared<CustomInspector>())
    {
        if (m_AppState == nullptr)
            return;

        m_CalculatedMaskGenerator = std::make_shared<CalculatedMaskGenerator>(m_AppState);
        m_MaskTool                = std::make_shared<MaskTool>(m_AppState, glm::vec3(1.0f, 0.65f, 0.1f));
        m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator->GetTexture(), "Calculated base-noise mask");

        std::string catalogError;
        if (!m_NoiseAlgorithms.LoadFromFile(NoiseAlgorithmCatalog::IndexPath(m_AppState->constants.shadersDir), &catalogError)) {
            TF3D_LOG_ERROR("{}", catalogError);
        }
    }

    bool BaseNoiseGenerator::LoadConfig(const nlohmann::json &config, const std::string &source, const std::string &shaderPath)
    {
        if (!config.is_object()) {
            TF3D_LOG_ERROR("Failed to load base-noise generator: metadata is not an object.");
            return false;
        }
        if (m_AppState == nullptr || m_AppState->resourceManager == nullptr) {
            TF3D_LOG_ERROR("Failed to load base-noise generator: application resources are unavailable.");
            return false;
        }

        m_ID          = config.value("ID", m_ID);
        m_Name        = config.value("Name", m_Name);
        m_Description = config.value("Description", "");
        m_Source      = source;
        m_ShaderPath  = shaderPath;

        auto inspectorConfig = config;
        if (!ApplyNoiseAlgorithmMetadata(inspectorConfig, m_NoiseAlgorithms)) {
            TF3D_LOG_ERROR("Failed to apply noise algorithm metadata for base-noise generator '{}'.", m_Name);
            return false;
        }
        if (!m_Inspector->LoadConfig(inspectorConfig))
            return false;

        m_Shader = m_AppState->resourceManager->GetComputeShader(
            "BaseNoiseGen_" + m_ID, m_NoiseAlgorithms.InjectShaderDefines(source));
        m_RequireUpdation = true;
        return m_Shader.has_value();
    }

    BaseNoiseGenerator::~BaseNoiseGenerator()
    {
    }

    bool BaseNoiseGenerator::ShowSettings()
    {
        if (m_Inspector == nullptr)
            return false;

        const auto markChanged = [this](bool changed) {
            m_RequireUpdation = changed || m_RequireUpdation;
        };

        ImGui::PushID(m_ID.c_str());
        if (!m_Description.empty() && m_Inspector->GetDescription().empty()) {
            ImGui::TextWrapped("%s", m_Description.c_str());
            ImGui::Separator();
        }
        markChanged(m_Inspector->Render());

        markChanged(ImGui::Checkbox("Use mask", &m_UseMask));
        if (m_UseMask && m_MaskTool != nullptr && m_CalculatedMaskGenerator != nullptr) {
            if (ImGui::CollapsingHeader("Mask Tool")) {
                markChanged(ImGui::Checkbox("Invert mask", &m_InvertMask));
                m_MaskTool->SetInvertPreview(m_InvertMask);
                m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator->GetTexture(), "Calculated base-noise mask");
                if (m_MaskTool->IsShowingGeneratedMask())
                    markChanged(m_CalculatedMaskGenerator->ShowSettings());
                markChanged(m_MaskTool->ShowSettings(true));
            }
        } else if (m_MaskTool != nullptr) {
            m_MaskTool->SetInvertPreview(false);
            ImGui::TextDisabled("Mask: Global");
        }
        ImGui::PopID();

        return m_RequireUpdation;
    }

    void BaseNoiseGenerator::Resize(int size)
    {
        if (size <= 0)
            return;
        if (m_CalculatedMaskGenerator != nullptr)
            m_CalculatedMaskGenerator->Resize(size);
        if (m_MaskTool != nullptr)
            m_MaskTool->Resize(size);
    }

    void BaseNoiseGenerator::Update(GeneratorData *sourceBuffer, GeneratorData *targetBuffer,
                                    GeneratorTexture *seedTexture, std::string_view profilePrefix)
    {
        if (!m_Shader || sourceBuffer == nullptr || targetBuffer == nullptr)
            return;

        const std::string scopePrefix = profilePrefix.empty() ? "generation" : std::string(profilePrefix);
        const std::string scopeKey    = scopePrefix + "/base-noise/" + m_Name;
        TF3D_PROFILE_SCOPE_LAZY_DOMAIN(scopeKey, PerformanceMonitor::Domain::Generation);

        const bool useMask = m_UseMask && m_MaskTool != nullptr && m_CalculatedMaskGenerator != nullptr &&
                             m_MaskTool->GetPreviewTexture() != nullptr;
        if (useMask) {
            m_CalculatedMaskGenerator->Invalidate();
            m_CalculatedMaskGenerator->Update(sourceBuffer);
            m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator->GetTexture(), "Calculated base-noise mask");
        }

        sourceBuffer->Bind(0);
        targetBuffer->Bind(1);

        m_Shader->Bind();
        m_Inspector->ApplyToShader(*m_Shader);
        m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
        m_Shader->SetUniform1i("u_UseSeedTexture", (seedTexture != nullptr && m_Inspector->Get("AutoUseSeedTexture", false)) ? 1 : 0);
        m_Shader->SetUniform1i("u_UseMask", useMask ? 1 : 0);
        m_Shader->SetUniform1i("u_InvertMask", useMask && m_InvertMask ? 1 : 0);
        if (seedTexture) {
            m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
        }
        if (useMask) {
            m_Shader->SetUniform1i("u_MaskTexture", m_MaskTool->GetPreviewTexture()->Bind(3));
        }
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (m_AppState->mainMap.tileResolution + workgroupSize - 1) / workgroupSize;
        const std::string gpuKey = scopeKey + "/gpu";
        TF3D_PROFILE_GPU_SCOPE(gpuKey);
        TF3D_PROFILE_VALUE_DOMAIN("generation/base-noise/dispatch", dispatchSize, dispatchSize, 1,
                                  PerformanceMonitor::Domain::Generation);
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();

        m_RequireUpdation = false;
    }

    void BaseNoiseGenerator::Load(SerializerNode data)
    {
        if (data == nullptr)
            return;

        m_Name        = data->Get<std::string>("Name", m_Name);
        m_ID          = data->Get<std::string>("ID", m_ID);
        m_Description = data->Get<std::string>("Description", m_Description);
        m_Source      = data->Get<std::string>("Source", m_Source);
        m_ShaderPath  = data->Get<std::string>("ShaderPath", m_ShaderPath);
        m_UseMask     = data->Get<bool>("UseMask", m_UseMask);
        m_InvertMask  = data->Get<bool>("InvertMask", m_InvertMask);

        auto inspector = data->Get<SerializerNode>("Inspector");
        if (inspector != nullptr) {
            auto inspectorState = inspector->ToJson();
            if (inspectorState.contains("Transform") && inspectorState["Transform"].is_object() &&
                inspectorState["Transform"].contains("Offset")) {
                if (!inspectorState.contains("Noise") || !inspectorState["Noise"].is_object())
                    inspectorState["Noise"] = nlohmann::json::object();
                if (!inspectorState["Noise"].contains("Offset"))
                    inspectorState["Noise"]["Offset"] = inspectorState["Transform"]["Offset"];
            }
            inspectorState.erase("Transform");
            inspectorState.erase("Filtering");
            inspectorState.erase("TransformFactor");
            inspectorState.erase("SlopeSmoothingRadius");
            inspectorState.erase("SlopeSamplingRadius");
            inspectorState.erase("TransformRange");
            inspectorState.erase("UseGaussianPreFilter");
            m_Inspector->LoadState(CreateSerializerNodeFromJson(inspectorState));
        }

        auto octaveStrengths            = m_Inspector->Get<std::vector<float>>("OctaveStrengths", DefaultNoiseOctaveStrengths());
        const auto savedOctaveStrengths = data->Get<std::vector<float>>("OctaveStrengths");
        if (!savedOctaveStrengths.empty()) {
            octaveStrengths = savedOctaveStrengths;
        } else if (!m_Inspector->Contains("OctaveStrengths")) {
            bool hasLegacyOctaves = false;
            octaveStrengths       = DefaultNoiseOctaveStrengths();
            for (int i = 0; i < kBaseNoiseOctaveCount; ++i) {
                const std::string variableName = "NoiseOctaveStrength" + std::to_string(i);
                if (!m_Inspector->Contains(variableName))
                    continue;
                hasLegacyOctaves                        = true;
                octaveStrengths[static_cast<size_t>(i)] = m_Inspector->Get(variableName, octaveStrengths[static_cast<size_t>(i)]);
            }
            if (!hasLegacyOctaves)
                octaveStrengths = DefaultNoiseOctaveStrengths();
        }
        octaveStrengths = NormalizeNoiseOctaveStrengths(octaveStrengths);
        if (m_Inspector->Contains("OctaveStrengths")) {
            if (!m_Inspector->Set("OctaveStrengths", octaveStrengths)) {
                m_Inspector->Remove("OctaveStrengths");
                m_Inspector->Add("OctaveStrengths", octaveStrengths);
            }
        } else {
            m_Inspector->Add("OctaveStrengths", octaveStrengths);
        }
        for (int i = 0; i < kBaseNoiseOctaveCount; ++i)
            m_Inspector->Remove("NoiseOctaveStrength" + std::to_string(i));
        if (!m_Inspector->Contains("NoiseOctaveStrengthsCount"))
            m_Inspector->Add("NoiseOctaveStrengthsCount", kBaseNoiseOctaveCount);
        EnsureNoiseOctaveComponent(*m_Inspector);
        EnsureNoiseValues(*m_Inspector, m_NoiseAlgorithms.DefaultValue());

        if (m_CalculatedMaskGenerator != nullptr) {
            m_CalculatedMaskGenerator->Load(data->Get<SerializerNode>("CalculatedMask"));
            m_CalculatedMaskGenerator->Invalidate();
        }
        if (m_MaskTool != nullptr) {
            m_MaskTool->SetGeneratedMaskTexture(m_CalculatedMaskGenerator != nullptr ? m_CalculatedMaskGenerator->GetTexture() : nullptr,
                                                "Calculated base-noise mask");
            m_MaskTool->Load(data->Get<SerializerNode>("MaskTool"));
            m_MaskTool->SetInvertPreview(m_InvertMask);
        }
        m_RequireUpdation = true;
    }

    SerializerNode BaseNoiseGenerator::Save()
    {
        auto octaveStrengths = NormalizeNoiseOctaveStrengths(
            m_Inspector->Get<std::vector<float>>("OctaveStrengths", DefaultNoiseOctaveStrengths()));
        if (m_Inspector->Contains("OctaveStrengths"))
            m_Inspector->Set("OctaveStrengths", octaveStrengths);
        else
            m_Inspector->Add("OctaveStrengths", octaveStrengths);

        auto node = CreateSerializerNode();
        node->Set("Name", m_Name);
        node->Set("ID", m_ID);
        node->Set("Description", m_Description);
        node->Set("Source", m_Source);
        node->Set("ShaderPath", m_ShaderPath);
        node->Set("UseMask", m_UseMask);
        node->Set("InvertMask", m_InvertMask);
        node->Set("Inspector", m_Inspector->SaveState());
        node->Set("OctaveStrengths", octaveStrengths);
        if (m_CalculatedMaskGenerator != nullptr)
            node->Set("CalculatedMask", m_CalculatedMaskGenerator->Save());
        if (m_MaskTool != nullptr)
            node->Set("MaskTool", m_MaskTool->Save());
        return node;
    }

} // namespace tf3d::generators
