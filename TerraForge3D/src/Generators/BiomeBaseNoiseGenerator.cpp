#include "Generators/BiomeBaseNoiseGenerator.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"
#include "Profiler.h"

BiomeBaseNoiseGenerator::BiomeBaseNoiseGenerator(ApplicationState* appState)
{
	m_AppState = appState;

	// const auto shaderSource = ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_noise" PATH_SEPARATOR "noise_gen.glsl", &s_TempBool);
	// m_Shader = std::make_shared<ComputeShader>(shaderSource);
	m_Shader = m_AppState->resourceManager->LoadComputeShader("generation/base_noise/noise_gen");

	m_Inspector = std::make_shared<CustomInspector>();


	std::fill_n(m_NoiseOctaveStrengths, BIOME_BASE_NOISE_OCTAVE_COUNT, 1.0f);
	m_NoiseOctaveStrengths[0] = m_NoiseOctaveStrengths[1] = 0.0f;

	{
		m_Inspector->AddIntegerVariable("Seed", 152);
		m_Inspector->AddSeedWidget("Seed", "Seed");

		m_Inspector->AddFloatVariable("Influence", 0.5f);
		m_Inspector->AddSliderWidget("Influence", "Influence", 0.0f, 1.0f);

		m_Inspector->AddFloatVariable("Strength", 1.0f);
		m_Inspector->AddDragWidget("Strength", "Strength", 0.0f, 0.0f, 0.01f);

		m_Inspector->AddFloatVariable("Frequency", 0.45f);
		m_Inspector->AddDragWidget("Frequency", "Frequency", 0.0f, 0.0f, 0.01f);

		m_Inspector->AddFloatVariable("Lacunarity", 1.8f);
		m_Inspector->AddDragWidget("Lacunarity", "Lacunarity", 0.0f, 0.0f, 0.01f);

		m_Inspector->AddFloatVariable("Persistence", 0.55f);
		m_Inspector->AddDragWidget("Persistence", "Persistence", 0.0f, 0.0f, 0.01f);

		m_Inspector->AddBoolVariable("AutoUseSeedTexture", true);
		m_Inspector->AddCheckboxWidget("Auto Use Seed Texture", "AutoUseSeedTexture").SetTooltip("Setting this to true will cause seed texture to be ignored");

		m_Inspector->AddVector3Variable("Offset");
		m_Inspector->AddDragWidget("Offset", "Offset", 0.0f, 0.0f, 0.01f);

		m_Inspector->AddIntegerVariable("MixMethod");
		m_Inspector->AddDropdownWidget("Mix Method", "MixMethod", { "Add", "Multiply", "Add & Multiply", "Set", "None"});

		m_Inspector->AddIntegerVariable("TransformFactor", 1);
		m_Inspector->AddDropdownWidget("Transform Factor", "TransformFactor", { "None", "Slope", "Height" });

		m_Inspector->AddIntegerVariable("SlopeSmoothingRadius", 3);
		m_Inspector->AddSliderWidget("Slope Smoothing Radius", "SlopeSmoothingRadius", 0, 20).SetRenderOnCondition("TransformFactor", 1);

		m_Inspector->AddFloatVariable("SlopeSamplingRadius", 3);
		m_Inspector->AddSliderWidget("Slope Sampling Radius", "SlopeSamplingRadius", 1.0, 10.0).SetRenderOnCondition("TransformFactor", 1);

		m_Inspector->AddVector2Variable("TransformRange", { 0.0f, 1.0f });
		m_Inspector->AddDragWidget("Transform Range", "TransformRange", 0.0f, 0.0f, 0.01f);

		m_Inspector->AddBoolVariable("UseGaussianPreFilter", false);
		m_Inspector->AddCheckboxWidget("Use Gaussian Pre Filter", "UseGaussianPreFilter");
	}
}

BiomeBaseNoiseGenerator::~BiomeBaseNoiseGenerator()
{

}

bool BiomeBaseNoiseGenerator::ShowSettings()
{
	if (ImGui::CollapsingHeader("Statistics"))
	{
		ImGui::Text("Time Take: %f", m_CalculationTime);
	}

	BIOME_UI_PROPERTY(m_Inspector->Render());

	ImGui::Text("Noise Octaves Strengths: ");
	ImGui::PushID("##NoiseOctaves");
	for (int i = 0; i < BIOME_BASE_NOISE_OCTAVE_COUNT; i++)
	{
		ImGui::PushID(i);
		// ImGui::SliderFloat("##Octave", &m_NoiseOctaveStrengths[i], 0.0f, 1.0f);
		BIOME_UI_PROPERTY(ImGui::VSliderFloat("##Octave", ImVec2(20, 200), &m_NoiseOctaveStrengths[i], 0.0f, 1.0f));
		ImGui::PopID();
		ImGui::SameLine();
	}
	ImGui::PopID();
	ImGui::NewLine();

	return m_RequireUpdation;
}

void BiomeBaseNoiseGenerator::Update(GeneratorData* sourceBuffer, GeneratorData* targetBuffer, GeneratorTexture* seedTexture)
{
	START_PROFILER();


	sourceBuffer->Bind(0);
	targetBuffer->Bind(1);	

	m_Shader->Bind();

	const auto& values = m_Inspector->GetValues();

	m_Shader->SetUniform1f("u_Strength", values.at("Strength").GetFloat());
	m_Shader->SetUniform1f("u_Influence", values.at("Influence").GetFloat());
	m_Shader->SetUniform1f("u_Frequency", values.at("Frequency").GetFloat());
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
	for (int i = 0; i < BIOME_BASE_NOISE_OCTAVE_COUNT; i++)
	{
		m_Shader->SetUniform1f("u_NoiseOctaveStrengths[" + std::to_string(i) + "]", m_NoiseOctaveStrengths[i]);
	}
	m_Shader->SetUniform1i("u_NoiseOctaveStrengthsCount", BIOME_BASE_NOISE_OCTAVE_COUNT);
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->SetUniform1i("u_UseSeedTexture", (seedTexture != nullptr && values.at("AutoUseSeedTexture").GetBool()) ? 1 : 0);
	if (seedTexture) m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
	const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	m_Shader->SetMemoryBarrier();

	END_PROFILER(m_CalculationTime);
	m_RequireUpdation = false;
}

void BiomeBaseNoiseGenerator::Load(SerializerNode data)
{
	m_Inspector->LoadData(data->GetChildNode("Inspector"));
	auto noiseOctavesVector = data->GetFloatArray("OctaveStrengths");
	for (int i = 0; i < BIOME_BASE_NOISE_OCTAVE_COUNT; i++) m_NoiseOctaveStrengths[i] = noiseOctavesVector[i];
}

SerializerNode BiomeBaseNoiseGenerator::Save()
{
	auto node = CreateSerializerNode();
	node->SetChildNode("Inspector", m_Inspector->SaveData());
	const auto noiseOctavesVector = std::vector<float>(m_NoiseOctaveStrengths, m_NoiseOctaveStrengths + BIOME_BASE_NOISE_OCTAVE_COUNT);
	node->SetFloatArray("OctaveStrengths", noiseOctavesVector);
	return node;
}
