#include "Generators/BiomeBaseNoiseGenerator.h"
#include "Data/ApplicationState.h"
#include "Generators/NoiseAlgorithmConfig.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

namespace tf3d::generators
{

    namespace
    {
        void EnsureNoiseValues(CustomInspector &inspector, int defaultNoiseAlgorithm)
        {
            if (!inspector.Contains("NoiseAlgorithm"))
                inspector.Add("NoiseAlgorithm", defaultNoiseAlgorithm);
            if (!inspector.Contains("NoiseOctaves"))
                inspector.Add("NoiseOctaves", 10);
            if (!inspector.Contains("NoiseWarp"))
                inspector.Add("NoiseWarp", 0.0f);
            if (!inspector.Contains("NoiseJitter"))
                inspector.Add("NoiseJitter", 0.75f);
        }

        std::vector<float> DefaultNoiseOctaveStrengths()
        {
            std::vector<float> values(BIOME_BASE_NOISE_OCTAVE_COUNT, 1.0f);
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

    BiomeBaseNoiseGenerator::BiomeBaseNoiseGenerator(ApplicationState *appState)
    {
        m_AppState  = appState;
        m_Inspector = std::make_shared<CustomInspector>();

        if (m_AppState == nullptr)
            return;

        std::string catalogError;
        if (!m_NoiseAlgorithms.LoadFromFile(NoiseAlgorithmCatalog::IndexPath(m_AppState->constants.shadersDir), &catalogError)) {
            TF3D_LOG_ERROR("{}", catalogError);
        }
    }

    bool BiomeBaseNoiseGenerator::LoadConfig(const nlohmann::json &config, const std::string &source, const std::string &shaderPath)
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

    BiomeBaseNoiseGenerator::~BiomeBaseNoiseGenerator()
    {
    }

    bool BiomeBaseNoiseGenerator::ShowSettings()
    {
        if (m_Inspector == nullptr)
            return false;

        ImGui::PushID(m_ID.c_str());
        if (!m_Description.empty() && m_Inspector->GetDescription().empty()) {
            ImGui::TextWrapped("%s", m_Description.c_str());
            ImGui::Separator();
        }
        BASE_NOISE_UI_PROPERTY(m_Inspector->Render());
        ImGui::PopID();

        return m_RequireUpdation;
    }

    void BiomeBaseNoiseGenerator::Update(GeneratorData *sourceBuffer, GeneratorData *targetBuffer, GeneratorTexture *seedTexture)
    {
        if (!m_Shader || sourceBuffer == nullptr || targetBuffer == nullptr)
            return;

        TF3D_PROFILE_SCOPE(std::string("generation/base-noise/") + m_Name);

        sourceBuffer->Bind(0);
        targetBuffer->Bind(1);

        m_Shader->Bind();
        m_Inspector->ApplyToShader(*m_Shader);
        m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
        m_Shader->SetUniform1i("u_UseSeedTexture", (seedTexture != nullptr && m_Inspector->Get("AutoUseSeedTexture", false)) ? 1 : 0);
        if (seedTexture)
            m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (m_AppState->mainMap.tileResolution + workgroupSize - 1) / workgroupSize;
        m_Shader->Dispatch(dispatchSize, dispatchSize, 1);
        m_Shader->SetMemoryBarrier();

        m_RequireUpdation = false;
    }

    void BiomeBaseNoiseGenerator::Load(SerializerNode data)
    {
        if (data == nullptr)
            return;

        m_Name        = data->Get<std::string>("Name", m_Name);
        m_ID          = data->Get<std::string>("ID", m_ID);
        m_Description = data->Get<std::string>("Description", m_Description);
        m_Source      = data->Get<std::string>("Source", m_Source);
        m_ShaderPath  = data->Get<std::string>("ShaderPath", m_ShaderPath);

        auto inspector = data->Get<SerializerNode>("Inspector");
        if (inspector != nullptr) {
            if (inspector->HasKey("Data"))
                m_Inspector->Load(inspector);
            else
                m_Inspector->LoadData(inspector);
        }

        auto octaveStrengths            = m_Inspector->Get<std::vector<float>>("OctaveStrengths", DefaultNoiseOctaveStrengths());
        const auto savedOctaveStrengths = data->Get<std::vector<float>>("OctaveStrengths");
        if (!savedOctaveStrengths.empty()) {
            octaveStrengths = savedOctaveStrengths;
        } else if (!m_Inspector->Contains("OctaveStrengths")) {
            bool hasLegacyOctaves = false;
            octaveStrengths       = DefaultNoiseOctaveStrengths();
            for (int i = 0; i < BIOME_BASE_NOISE_OCTAVE_COUNT; ++i) {
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
        for (int i = 0; i < BIOME_BASE_NOISE_OCTAVE_COUNT; ++i)
            m_Inspector->Remove("NoiseOctaveStrength" + std::to_string(i));
        if (!m_Inspector->Contains("NoiseOctaveStrengthsCount"))
            m_Inspector->Add("NoiseOctaveStrengthsCount", BIOME_BASE_NOISE_OCTAVE_COUNT);
        EnsureNoiseOctaveComponent(*m_Inspector);
        EnsureNoiseValues(*m_Inspector, m_NoiseAlgorithms.DefaultValue());
        m_RequireUpdation = true;
    }

    SerializerNode BiomeBaseNoiseGenerator::Save()
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
        node->Set("Inspector", m_Inspector->Save());
        node->Set("OctaveStrengths", octaveStrengths);
        return node;
    }

} // namespace tf3d::generators
