#include "Generators/BiomeBaseShape/BiomeBaseShape_Cliff.h"
#include "Data/ApplicationState.h"

BiomeBaseShape_Cliff::BiomeBaseShape_Cliff(ApplicationState* appState)
	: BiomeBaseShapeGenerator(appState, "Cliff")
{
	s_TempBool = false;
	auto path = m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "cliff.glsl";
	auto source = ReadShaderSourceFile(path, &s_TempBool);
	if (!s_TempBool)
	{
		Log("Failed to load shader source file: %s" + path);
		return;
	}
	m_Shader = new ComputeShader(source);
}

BiomeBaseShape_Cliff::~BiomeBaseShape_Cliff()
{
	if (m_Shader) delete m_Shader;
}
 
bool BiomeBaseShape_Cliff::ShowSettings()
{
	BIOME_UI_PROPERTY(ShowSeedSettings("Seed", &m_Seed, m_SeedHistory));
	BIOME_UI_PROPERTY(ImGui::SliderFloat("Strength", &m_Strength, 0.0f, 1.0f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("NoiseScale", &m_NoiseScale, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("NoiseStrength", &m_NoiseStrength, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("DistortionScale", &m_DistortionScale, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("DistortionStrength", &m_DistortionStrength, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Smoothness", &m_Smoothness, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Height", &m_Height, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Position", &m_Position, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Rotation", &m_Rotation, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Thickness", &m_Thickness, 0.01f));

	   
	return m_RequireUpdation;
}

void BiomeBaseShape_Cliff::Update(GeneratorData* buffer, GeneratorTexture* seedTexture)
{
	auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	buffer->Bind(0);
	m_Shader->Bind();
	m_Shader->SetUniform1f("u_Strength", m_Strength);
	m_Shader->SetUniform1f("u_NoiseScale", m_NoiseScale);
	m_Shader->SetUniform1f("u_NoiseStrength", m_NoiseStrength);
	m_Shader->SetUniform1f("u_DistortionScale", m_DistortionScale);
	m_Shader->SetUniform1f("u_DistortionStrength", m_DistortionStrength);
	m_Shader->SetUniform1f("u_Smoothness", m_Smoothness);
	m_Shader->SetUniform1f("u_Height", m_Height);
	m_Shader->SetUniform1f("u_Position", m_Position);
	m_Shader->SetUniform1f("u_Rotation", m_Rotation);
	m_Shader->SetUniform1f("u_Thickness", m_Thickness);
	m_Shader->SetUniform1i("u_Seed", m_Seed);
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->SetUniform1i("u_UseSeedTexture", seedTexture != nullptr);
	if (seedTexture) m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	m_RequireUpdation = false;
}

nlohmann::json BiomeBaseShape_Cliff::OnSave()
{
	nlohmann::json data;
	// data["m_SubStyle"] = m_SubStyle;
	return data;
}

void BiomeBaseShape_Cliff::OnLoad(nlohmann::json data)
{
	// if(data.contains("m_Height")) m_Height = data["m_Height"]; else m_Height = 0.5f;
	m_RequireUpdation = true;
}
