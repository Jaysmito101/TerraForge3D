#include "Generators/BiomeBaseNoiseGenerator.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

namespace tf3d::generators
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

    BiomeBaseNoiseGenerator::BiomeBaseNoiseGenerator(ApplicationState *appState)
    {
        m_AppState = appState;
        std::string catalogError;
        if (!m_NoiseAlgorithms.LoadFromFile(NoiseAlgorithmCatalog::IndexPath(m_AppState->constants.shadersDir), &catalogError))
            TF3D_LOG_ERROR("{}", catalogError);

        bool shaderLoaded       = false;
        const auto shaderSource = m_AppState->resourceManager->LoadShaderSource("generation/base_noise/noise_gen", false, &shaderLoaded);
        if (shaderLoaded && m_NoiseAlgorithms.IsValid()) {
            m_Shader = m_AppState->resourceManager->GetComputeShader(
                "generation/base_noise/noise_gen", m_NoiseAlgorithms.InjectShaderDefines(shaderSource));
        }

        m_Inspector = std::make_shared<CustomInspector>();

        m_NoiseOctaveStrengths.fill(1.0f);
        m_NoiseOctaveStrengths[0] = m_NoiseOctaveStrengths[1] = 0.0f;
        {
            m_Inspector->Add("Seed", 152);
            auto &seedWidget = m_Inspector->AddWidget("Noise Seed", CustomInspectorWidgetType::Seed, "Seed");
            seedWidget.SetTooltip("Random seed used to generate the base noise pattern.");

            m_Inspector->Add("NoiseAlgorithm", m_NoiseAlgorithms.DefaultValue());
            auto &algorithmWidget = m_Inspector->AddWidget("Noise Algorithm", CustomInspectorWidgetType::Dropdown, "NoiseAlgorithm");
            algorithmWidget.SetDropdownOptions(m_NoiseAlgorithms.Labels());
            algorithmWidget.SetTooltip(m_NoiseAlgorithms.Tooltip());

            m_Inspector->Add("Influence", 0.5f);
            auto &influenceWidget = m_Inspector->AddWidget("Influence", CustomInspectorWidgetType::Slider, "Influence");
            influenceWidget.SetConstraints(0.0f, 1.0f);
            influenceWidget.SetTooltip("Controls how strongly this generator contributes to the final biome output.");

            m_Inspector->Add("Strength", 1.0f);
            auto &strengthWidget = m_Inspector->AddWidget("Strength", CustomInspectorWidgetType::Drag, "Strength");
            strengthWidget.SetConstraints(0.0f, 0.0f);
            strengthWidget.SetSpeed(0.01f);
            strengthWidget.SetTooltip("Scales the amplitude of the generated noise.");

            m_Inspector->Add("Frequency", 0.45f);
            auto &frequencyWidget = m_Inspector->AddWidget("Noise Scale", CustomInspectorWidgetType::Drag, "Frequency");
            frequencyWidget.SetConstraints(0.0f, 0.0f);
            frequencyWidget.SetSpeed(0.001f);
            frequencyWidget.SetTooltip("Controls the size of the shared noise features. Higher values create smaller features.");

            m_Inspector->Add("Lacunarity", 1.8f);
            auto &lacunarityWidget = m_Inspector->AddWidget("Noise Lacunarity", CustomInspectorWidgetType::Drag, "Lacunarity");
            lacunarityWidget.SetConstraints(1.0f, 4.0f);
            lacunarityWidget.SetSpeed(0.01f);
            lacunarityWidget.SetTooltip("Frequency multiplier between successive noise layers.");

            m_Inspector->Add("Persistence", 0.55f);
            auto &persistenceWidget = m_Inspector->AddWidget("Noise Persistence", CustomInspectorWidgetType::Slider, "Persistence");
            persistenceWidget.SetConstraints(0.0f, 0.99f);
            persistenceWidget.SetTooltip("Amplitude retained by each successive noise layer.");

            m_Inspector->Add("NoiseOctaves", 10);
            auto &octaveWidget = m_Inspector->AddWidget("Noise Octaves", CustomInspectorWidgetType::Slider, "NoiseOctaves");
            octaveWidget.SetConstraints(1.0f, static_cast<float>(BIOME_BASE_NOISE_OCTAVE_COUNT));
            octaveWidget.SetTooltip("Number of noise layers enabled from the octave strength profile.");

            m_Inspector->Add("NoiseWarp", 0.0f);
            auto &warpWidget = m_Inspector->AddWidget("Noise Warp", CustomInspectorWidgetType::Drag, "NoiseWarp");
            warpWidget.SetConstraints(0.0f, 4.0f);
            warpWidget.SetSpeed(0.01f);
            warpWidget.SetTooltip("Bends the sampling domain before evaluating the noise, adding organic distortion.");

            m_Inspector->Add("NoiseJitter", 0.75f);
            auto &jitterWidget = m_Inspector->AddWidget("Noise Jitter", CustomInspectorWidgetType::Slider, "NoiseJitter");
            jitterWidget.SetConstraints(0.0f, 1.0f);
            jitterWidget.SetTooltip("Moves feature points inside Voronoi, Worley, and Gabor cells.");
            jitterWidget.SetRenderOnConditions("NoiseAlgorithm", {m_NoiseAlgorithms.Value("Gabor"),
                                                                  m_NoiseAlgorithms.Value("Voronoi"),
                                                                  m_NoiseAlgorithms.Value("Worley")});

            m_Inspector->Add("AutoUseSeedTexture", true);
            auto &seedTextureWidget = m_Inspector->AddWidget("Auto Use Seed Texture", CustomInspectorWidgetType::Checkbox, "AutoUseSeedTexture");
            seedTextureWidget.SetTooltip("When enabled, the generated seed texture is ignored.");

            m_Inspector->Add<glm::vec3>("Offset");
            auto &offsetWidget = m_Inspector->AddWidget("Offset", CustomInspectorWidgetType::Drag, "Offset");
            offsetWidget.SetConstraints(0.0f, 0.0f);
            offsetWidget.SetSpeed(0.01f);
            offsetWidget.SetTooltip("Offsets the noise sampling position in 3D space.");

            m_Inspector->Add("MixMethod", 0);
            auto &mixMethodWidget = m_Inspector->AddWidget("Mix Method", CustomInspectorWidgetType::Dropdown, "MixMethod");
            mixMethodWidget.SetDropdownOptions({"Add", "Multiply", "Add & Multiply", "Set", "None"});
            mixMethodWidget.SetTooltip("Selects how this output is blended with the source buffer.");

            m_Inspector->Add("TransformFactor", 1);
            auto &transformFactorWidget = m_Inspector->AddWidget("Transform Factor", CustomInspectorWidgetType::Dropdown, "TransformFactor");
            transformFactorWidget.SetDropdownOptions({"None", "Slope", "Height"});
            transformFactorWidget.SetTooltip("Chooses which terrain factor is used to remap the noise.");

            m_Inspector->Add("SlopeSmoothingRadius", 3);
            auto &slopeSmoothingWidget = m_Inspector->AddWidget("Slope Smoothing Radius", CustomInspectorWidgetType::Slider, "SlopeSmoothingRadius");
            slopeSmoothingWidget.SetConstraints(0.0f, 20.0f);
            slopeSmoothingWidget.SetTooltip("Sets the radius used to smooth slope sampling.");
            slopeSmoothingWidget.SetRenderOnCondition("TransformFactor", 1);

            m_Inspector->Add("SlopeSamplingRadius", 3.0f);
            auto &slopeSamplingWidget = m_Inspector->AddWidget("Slope Sampling Radius", CustomInspectorWidgetType::Slider, "SlopeSamplingRadius");
            slopeSamplingWidget.SetConstraints(1.0f, 10.0f);
            slopeSamplingWidget.SetTooltip("Controls how far the generator samples around each point when measuring slope.");
            slopeSamplingWidget.SetRenderOnCondition("TransformFactor", 1);

            m_Inspector->Add("TransformRange", glm::vec2(0.0f, 1.0f));
            auto &transformRangeWidget = m_Inspector->AddWidget("Transform Range", CustomInspectorWidgetType::Drag, "TransformRange");
            transformRangeWidget.SetConstraints(0.0f, 0.0f);
            transformRangeWidget.SetSpeed(0.001f);
            transformRangeWidget.SetTooltip("Remaps the chosen transform factor into this normalized range.");

            m_Inspector->Add("UseGaussianPreFilter", false);
            auto &gaussianFilterWidget = m_Inspector->AddWidget("Use Gaussian Pre Filter", CustomInspectorWidgetType::Checkbox, "UseGaussianPreFilter");
            gaussianFilterWidget.SetTooltip("Applies a gaussian pre-filter before the noise is evaluated.");
        }
    }

    BiomeBaseNoiseGenerator::~BiomeBaseNoiseGenerator()
    {
    }

    bool BiomeBaseNoiseGenerator::ShowSettings()
    {
        BIOME_UI_PROPERTY(m_Inspector->Render());

        ImGui::Text("Noise Octaves Strengths: ");
        ImGui::PushID("##NoiseOctaves");
        for (int i = 0; i < BIOME_BASE_NOISE_OCTAVE_COUNT; i++) {
            ImGui::PushID(i);
            BIOME_UI_PROPERTY(ImGui::VSliderFloat("##Octave", ImVec2(20, 200), &m_NoiseOctaveStrengths[i], 0.0f, 1.0f));
            ImGui::PopID();
            ImGui::SameLine();
        }
        ImGui::PopID();
        ImGui::NewLine();

        return m_RequireUpdation;
    }

    void BiomeBaseNoiseGenerator::Update(GeneratorData *sourceBuffer, GeneratorData *targetBuffer, GeneratorTexture *seedTexture)
    {
        TF3D_PROFILE_SCOPE("generation/base-noise");

        sourceBuffer->Bind(0);
        targetBuffer->Bind(1);

        m_Shader->Bind();

        m_Shader->SetUniform1f("u_Strength", m_Inspector->Get("Strength", 0.0f));
        m_Shader->SetUniform1f("u_Influence", m_Inspector->Get("Influence", 0.0f));
        m_Shader->SetUniform1f("u_Frequency", m_Inspector->Get("Frequency", 0.0f));
        m_Shader->SetUniform1i("u_NoiseAlgorithm", m_Inspector->Get("NoiseAlgorithm", 0));
        m_Shader->SetUniform1f("u_NoiseScale", m_Inspector->Get("Frequency", 0.0f));
        m_Shader->SetUniform1f("u_NoiseSeed", static_cast<float>(m_Inspector->Get("Seed", 0)));
        m_Shader->SetUniform1i("u_NoiseOctaves", m_Inspector->Get("NoiseOctaves", 0));
        m_Shader->SetUniform1f("u_NoiseWarp", m_Inspector->Get("NoiseWarp", 0.0f));
        m_Shader->SetUniform1f("u_NoiseJitter", m_Inspector->Get("NoiseJitter", 0.0f));
        m_Shader->SetUniform1f("u_Lacunarity", m_Inspector->Get("Lacunarity", 0.0f));
        m_Shader->SetUniform1f("u_Persistence", m_Inspector->Get("Persistence", 0.0f));
        m_Shader->SetUniform3f("u_Offset", m_Inspector->Get("Offset", glm::vec3(0.0f)));
        m_Shader->SetUniform1i("u_MixMethod", m_Inspector->Get("MixMethod", 0));
        m_Shader->SetUniform1i("u_TransformFactor", m_Inspector->Get("TransformFactor", 0));
        m_Shader->SetUniform1i("u_SlopeSmoothingRadius", m_Inspector->Get("SlopeSmoothingRadius", 0));
        m_Shader->SetUniform2f("u_TransformRange", m_Inspector->Get("TransformRange", glm::vec2(0.0f)));
        m_Shader->SetUniform1i("u_Seed", m_Inspector->Get("Seed", 0));
        m_Shader->SetUniform1f("u_SlopeSamplingRadius", m_Inspector->Get("SlopeSamplingRadius", 0.0f));
        m_Shader->SetUniform1i("u_UseGaussianPreFilter", m_Inspector->Get("UseGaussianPreFilter", false) ? 1 : 0);
        for (int i = 0; i < BIOME_BASE_NOISE_OCTAVE_COUNT; i++) {
            m_Shader->SetUniform1f("u_NoiseOctaveStrengths[" + std::to_string(i) + "]", m_NoiseOctaveStrengths[i]);
        }
        m_Shader->SetUniform1i("u_NoiseOctaveStrengthsCount", BIOME_BASE_NOISE_OCTAVE_COUNT);
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
        m_Inspector->LoadData(data->Get<SerializerNode>("Inspector"));
        auto noiseOctavesVector = data->Get<std::vector<float>>("OctaveStrengths");
        for (int i = 0; i < BIOME_BASE_NOISE_OCTAVE_COUNT; i++) {
            const std::string variableName = "NoiseOctaveStrength" + std::to_string(i);
            const float defaultValue       = i < 2 ? 0.0f : 1.0f;
            const float inspectorValue     = m_Inspector->Get(variableName, defaultValue);
            const float savedValue         = i < static_cast<int>(noiseOctavesVector.size()) ? noiseOctavesVector[i] : inspectorValue;
            m_NoiseOctaveStrengths[i]      = glm::clamp(savedValue, 0.0f, 1.0f);
            // Remove values written by the previous generic CustomInspector octave UI.
            if (m_Inspector->Contains(variableName))
                m_Inspector->Remove(variableName);
        }
        EnsureNoiseValues(*m_Inspector, m_NoiseAlgorithms.DefaultValue());
    }

    SerializerNode BiomeBaseNoiseGenerator::Save()
    {
        auto node = CreateSerializerNode();
        node->Set("Inspector", m_Inspector->SaveData());
        const auto noiseOctavesVector = std::vector<float>(m_NoiseOctaveStrengths.begin(), m_NoiseOctaveStrengths.end());
        node->Set("OctaveStrengths", noiseOctavesVector);
        return node;
    }

} // namespace tf3d::generators
