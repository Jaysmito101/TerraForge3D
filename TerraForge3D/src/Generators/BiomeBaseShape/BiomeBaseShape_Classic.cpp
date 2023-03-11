#include "Generators/BiomeBaseShape/BiomeBaseShape_Classic.h"
#include "Data/ApplicationState.h"

BiomeBaseShape_Classic::BiomeBaseShape_Classic(ApplicationState* appState)
	: BiomeBaseShapeGenerator(appState, "Classic") 
{
	s_TempBool = false;
	auto path = m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "classic.glsl";
	auto source = ReadShaderSourceFile(path, &s_TempBool);
	if (!s_TempBool)
	{
		Log("Failed to load shader source file: %s" + path);
		return;
	}
	m_Shader = new ComputeShader(source);
}

BiomeBaseShape_Classic::~BiomeBaseShape_Classic()
{
	if (m_Shader) delete m_Shader;
}

bool BiomeBaseShape_Classic::ShowSettings()
{
	BIOME_UI_PROPERTY(ImGui::Checkbox("Absolute Value", &m_AbsoluteValue));
	ImGui::SameLine();
	BIOME_UI_PROPERTY(ImGui::Checkbox("Square Value", &m_SquareValue));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Strength", &m_Strength, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Scale", &m_Scale, 0.01f));
	BIOME_UI_PROPERTY(ImGui::SliderInt("Levels", &m_Levels, 1, 8));
	//BIOME_UI_PROPERTY(ImGui::SliderInt("Seed", &m_Seed, 0, 100));
	BIOME_UI_PROPERTY(ShowSeedSettings("Seed", &m_Seed, m_SeedHistory));
	BIOME_UI_PROPERTY(ImGui::DragFloat3("Offset", m_Offset, 0.01f));
	return m_RequireUpdation; 
} 

void BiomeBaseShape_Classic::Update(GeneratorData* buffer, GeneratorTexture* seedTexture)
{
	auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	buffer->Bind(0);
	m_Shader->Bind();
	m_Shader->SetUniform1f("u_Strength", m_Strength);
	m_Shader->SetUniform1f("u_Scale", m_Scale);
	m_Shader->SetUniform1i("u_Levels", m_Levels);
	m_Shader->SetUniform1i("u_Seed", m_Seed);
	m_Shader->SetUniform1i("u_SquareValue", m_SquareValue);
	m_Shader->SetUniform1i("u_AbsoluteValue", m_AbsoluteValue);
	m_Shader->SetUniform3f("u_Offset", m_Offset[0], m_Offset[1], m_Offset[2]);
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->SetUniform1i("u_UseSeedTexture", seedTexture != nullptr);
	if (seedTexture) m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	m_RequireUpdation = false;
} 

nlohmann::json BiomeBaseShape_Classic::OnSave()
{
	nlohmann::json data;
	// data["m_SubStyle"] = m_SubStyle;
	return data;
}

void BiomeBaseShape_Classic::OnLoad(nlohmann::json data)
{
	// if(data.contains("m_Height")) m_Height = data["m_Height"]; else m_Height = 0.5f;
	m_RequireUpdation = true;
}
