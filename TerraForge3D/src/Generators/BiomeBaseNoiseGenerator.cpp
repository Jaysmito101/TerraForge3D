#include "Generators/BiomeBaseNoiseGenerator.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

namespace tf3d::generators
{

    void EnsureNoiseValues(CustomInspector &inspector, int defaultNoiseAlgorithm)
    {
        if (!inspector.HasVariable("NoiseAlgorithm"))
            inspector.AddIntegerVariable("NoiseAlgorithm", defaultNoiseAlgorithm);
        if (!inspector.HasVariable("NoiseOctaves"))
            inspector.AddIntegerVariable("NoiseOctaves", 10);
        if (!inspector.HasVariable("NoiseWarp"))
            inspector.AddFloatVariable("NoiseWarp", 0.0f);
        if (!inspector.HasVariable("NoiseJitter"))
            inspector.AddFloatVariable("NoiseJitter", 0.75f);
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
            m_Inspector->AddIntegerVariable("Seed", 152);
            auto &seedWidget = m_Inspector->AddSeedWidget("Noise Seed", "Seed");
            seedWidget.SetTooltip("Random seed used to generate the base noise pattern.");

            m_Inspector->AddIntegerVariable("NoiseAlgorithm", m_NoiseAlgorithms.DefaultValue());
            auto &algorithmWidget = m_Inspector->AddDropdownWidget("Noise Algorithm", "NoiseAlgorithm", m_NoiseAlgorithms.Labels());
            algorithmWidget.SetTooltip(m_NoiseAlgorithms.Tooltip());

            m_Inspector->AddFloatVariable("Influence", 0.5f);
            auto &influenceWidget = m_Inspector->AddSliderWidget("Influence", "Influence", 0.0f, 1.0f);
            influenceWidget.SetTooltip("Controls how strongly this generator contributes to the final biome output.");

            m_Inspector->AddFloatVariable("Strength", 1.0f);
            auto &strengthWidget = m_Inspector->AddDragWidget("Strength", "Strength", 0.0f, 0.0f, 0.01f);
            strengthWidget.SetTooltip("Scales the amplitude of the generated noise.");

            m_Inspector->AddFloatVariable("Frequency", 0.45f);
            auto &frequencyWidget = m_Inspector->AddDragWidget("Noise Scale", "Frequency", 0.0f, 0.0f, 0.001f);
            frequencyWidget.SetTooltip("Controls the size of the shared noise features. Higher values create smaller features.");

            m_Inspector->AddFloatVariable("Lacunarity", 1.8f);
            auto &lacunarityWidget = m_Inspector->AddDragWidget("Noise Lacunarity", "Lacunarity", 1.0f, 4.0f, 0.01f);
            lacunarityWidget.SetTooltip("Frequency multiplier between successive noise layers.");

            m_Inspector->AddFloatVariable("Persistence", 0.55f);
            auto &persistenceWidget = m_Inspector->AddSliderWidget("Noise Persistence", "Persistence", 0.0f, 0.99f);
            persistenceWidget.SetTooltip("Amplitude retained by each successive noise layer.");

            m_Inspector->AddIntegerVariable("NoiseOctaves", 10);
            auto &octaveWidget = m_Inspector->AddSliderWidget("Noise Octaves", "NoiseOctaves", 1, BIOME_BASE_NOISE_OCTAVE_COUNT);
            octaveWidget.SetTooltip("Number of noise layers enabled from the octave strength profile.");

            m_Inspector->AddFloatVariable("NoiseWarp", 0.0f);
            auto &warpWidget = m_Inspector->AddDragWidget("Noise Warp", "NoiseWarp", 0.0f, 4.0f, 0.01f);
            warpWidget.SetTooltip("Bends the sampling domain before evaluating the noise, adding organic distortion.");

            m_Inspector->AddFloatVariable("NoiseJitter", 0.75f);
            auto &jitterWidget = m_Inspector->AddSliderWidget("Noise Jitter", "NoiseJitter", 0.0f, 1.0f);
            jitterWidget.SetTooltip("Moves feature points inside Voronoi, Worley, and Gabor cells.");
            jitterWidget.SetRenderOnConditions("NoiseAlgorithm", {m_NoiseAlgorithms.Value("Gabor"),
                                                                  m_NoiseAlgorithms.Value("Voronoi"),
                                                                  m_NoiseAlgorithms.Value("Worley")});

            m_Inspector->AddBoolVariable("AutoUseSeedTexture", true);
            auto &seedTextureWidget = m_Inspector->AddCheckboxWidget("Auto Use Seed Texture", "AutoUseSeedTexture");
            seedTextureWidget.SetTooltip("When enabled, the generated seed texture is ignored.");

            m_Inspector->AddVector3Variable("Offset");
            auto &offsetWidget = m_Inspector->AddDragWidget("Offset", "Offset", 0.0f, 0.0f, 0.01f);
            offsetWidget.SetTooltip("Offsets the noise sampling position in 3D space.");

            m_Inspector->AddIntegerVariable("MixMethod");
            auto &mixMethodWidget = m_Inspector->AddDropdownWidget("Mix Method", "MixMethod", {"Add", "Multiply", "Add & Multiply", "Set", "None"});
            mixMethodWidget.SetTooltip("Selects how this output is blended with the source buffer.");

            m_Inspector->AddIntegerVariable("TransformFactor", 1);
            auto &transformFactorWidget = m_Inspector->AddDropdownWidget("Transform Factor", "TransformFactor", {"None", "Slope", "Height"});
            transformFactorWidget.SetTooltip("Chooses which terrain factor is used to remap the noise.");

            m_Inspector->AddIntegerVariable("SlopeSmoothingRadius", 3);
            auto &slopeSmoothingWidget = m_Inspector->AddSliderWidget("Slope Smoothing Radius", "SlopeSmoothingRadius", 0, 20);
            slopeSmoothingWidget.SetTooltip("Sets the radius used to smooth slope sampling.");
            slopeSmoothingWidget.SetRenderOnCondition("TransformFactor", 1);

            m_Inspector->AddFloatVariable("SlopeSamplingRadius", 3);
            auto &slopeSamplingWidget = m_Inspector->AddSliderWidget("Slope Sampling Radius", "SlopeSamplingRadius", 1.0, 10.0);
            slopeSamplingWidget.SetTooltip("Controls how far the generator samples around each point when measuring slope.");
            slopeSamplingWidget.SetRenderOnCondition("TransformFactor", 1);

            m_Inspector->AddVector2Variable("TransformRange", {0.0f, 1.0f});
            auto &transformRangeWidget = m_Inspector->AddDragWidget("Transform Range", "TransformRange", 0.0f, 0.0f, 0.001f);
            transformRangeWidget.SetTooltip("Remaps the chosen transform factor into this normalized range.");

            m_Inspector->AddBoolVariable("UseGaussianPreFilter", false);
            auto &gaussianFilterWidget = m_Inspector->AddCheckboxWidget("Use Gaussian Pre Filter", "UseGaussianPreFilter");
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

        const auto &values = m_Inspector->GetValues();

        m_Shader->SetUniform1f("u_Strength", values.at("Strength").GetFloat());
        m_Shader->SetUniform1f("u_Influence", values.at("Influence").GetFloat());
        m_Shader->SetUniform1f("u_Frequency", values.at("Frequency").GetFloat());
        m_Shader->SetUniform1i("u_NoiseAlgorithm", values.at("NoiseAlgorithm").GetInt());
        m_Shader->SetUniform1f("u_NoiseScale", values.at("Frequency").GetFloat());
        m_Shader->SetUniform1f("u_NoiseSeed", static_cast<float>(values.at("Seed").GetInt()));
        m_Shader->SetUniform1i("u_NoiseOctaves", values.at("NoiseOctaves").GetInt());
        m_Shader->SetUniform1f("u_NoiseWarp", values.at("NoiseWarp").GetFloat());
        m_Shader->SetUniform1f("u_NoiseJitter", values.at("NoiseJitter").GetFloat());
        m_Shader->SetUniform1f("u_Lacunarity", values.at("Lacunarity").GetFloat());
        m_Shader->SetUniform1f("u_Persistence", values.at("Persistence").GetFloat());
        m_Shader->SetUniform3f("u_Offset", values.at("Offset").GetVector3());
        m_Shader->SetUniform1i("u_MixMethod", values.at("MixMethod").GetInt());
        m_Shader->SetUniform1i("u_TransformFactor", values.at("TransformFactor").GetInt());
        m_Shader->SetUniform1i("u_SlopeSmoothingRadius", values.at("SlopeSmoothingRadius").GetInt());
        m_Shader->SetUniform2f("u_TransformRange", values.at("TransformRange").GetVector2());
        m_Shader->SetUniform1i("u_Seed", values.at("Seed").GetInt());
        m_Shader->SetUniform1f("u_SlopeSamplingRadius", values.at("SlopeSamplingRadius").GetFloat());
        m_Shader->SetUniform1i("u_UseGaussianPreFilter", values.at("UseGaussianPreFilter").GetInt());
        for (int i = 0; i < BIOME_BASE_NOISE_OCTAVE_COUNT; i++) {
            m_Shader->SetUniform1f("u_NoiseOctaveStrengths[" + std::to_string(i) + "]", m_NoiseOctaveStrengths[i]);
        }
        m_Shader->SetUniform1i("u_NoiseOctaveStrengthsCount", BIOME_BASE_NOISE_OCTAVE_COUNT);
        m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
        m_Shader->SetUniform1i("u_UseSeedTexture", (seedTexture != nullptr && values.at("AutoUseSeedTexture").GetBool()) ? 1 : 0);
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
        m_Inspector->LoadData(data->GetChildNode("Inspector"));
        auto noiseOctavesVector = data->GetFloatArray("OctaveStrengths");
        for (int i = 0; i < BIOME_BASE_NOISE_OCTAVE_COUNT; i++) {
            const std::string variableName = "NoiseOctaveStrength" + std::to_string(i);
            const float defaultValue       = i < 2 ? 0.0f : 1.0f;
            const float inspectorValue     = m_Inspector->HasVariable(variableName) ? m_Inspector->GetValues().at(variableName).GetFloat() : defaultValue;
            const float savedValue         = i < static_cast<int>(noiseOctavesVector.size()) ? noiseOctavesVector[i] : inspectorValue;
            m_NoiseOctaveStrengths[i]      = glm::clamp(savedValue, 0.0f, 1.0f);
            // Remove values written by the previous generic CustomInspector octave UI.
            if (m_Inspector->HasVariable(variableName))
                m_Inspector->RemoveVariable(variableName);
        }
        EnsureNoiseValues(*m_Inspector, m_NoiseAlgorithms.DefaultValue());
    }

    SerializerNode BiomeBaseNoiseGenerator::Save()
    {
        auto node = CreateSerializerNode();
        node->SetChildNode("Inspector", m_Inspector->SaveData());
        const auto noiseOctavesVector = std::vector<float>(m_NoiseOctaveStrengths.begin(), m_NoiseOctaveStrengths.end());
        node->SetFloatArray("OctaveStrengths", noiseOctavesVector);
        return node;
    }

} // namespace tf3d::generators
