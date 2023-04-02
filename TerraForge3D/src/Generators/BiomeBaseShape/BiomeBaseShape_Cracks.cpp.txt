#include "Generators/BiomeBaseShape/BiomeBaseShape_Cracks.h"
#include "Data/ApplicationState.h"

BiomeBaseShape_Cracks::BiomeBaseShape_Cracks(ApplicationState* appState)
	: BiomeBaseShapeGenerator(appState, "Cracks")
{
	s_TempBool = false; 
	auto path = m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "Cracks.glsl";
	auto source = ReadShaderSourceFile(path, &s_TempBool);
	if (!s_TempBool)
	{
		Log("Failed to load shader source file: %s" + path);
		return;
	}
	m_Shader = new ComputeShader(source);
} 

BiomeBaseShape_Cracks::~BiomeBaseShape_Cracks()
{
	if (m_Shader) delete m_Shader;
}  

bool BiomeBaseShape_Cracks::ShowSettings()
{    
	BIOME_UI_PROPERTY(ImGui::Checkbox("Absolute Value", &m_AbsoluteValue));
	ImGui::SameLine();
	BIOME_UI_PROPERTY(ImGui::Checkbox("Square Value", &m_SquareValue));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Strength", &m_Strength, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Random Heights", &m_RandomHeights, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Crack Shape Distortion", &m_CrackShapeDistortion, 0.01f, 0.0f, 3.0f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Scale", &m_Scale, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Smoothness", &m_Smoothness, 0.01f, 0.0f, 1.0f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Noise Strength", &m_NoiseStrength, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat("Noise Scale", &m_NoiseScale, 0.01f));
	BIOME_UI_PROPERTY(ImGui::DragFloat2("Min Max Height", m_MinMaxHeight, 0.01f));
	BIOME_UI_PROPERTY(ShowSeedSettings("Seed", &m_Seed, m_SeedHistory));
	BIOME_UI_PROPERTY(ImGui::DragFloat3("Offset", m_Offset, 0.01f));
	return m_RequireUpdation;
} 
      
void BiomeBaseShape_Cracks::Update(GeneratorData* buffer, GeneratorTexture* seedTexture)
{
	auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	buffer->Bind(0); 
	m_Shader->Bind();
	m_Shader->SetUniform1f("u_Strength", m_Strength);
	m_Shader->SetUniform1f("u_Scale", m_Scale);
	m_Shader->SetUniform1f("u_NoiseScale", m_NoiseScale);
	m_Shader->SetUniform1f("u_NoiseStrength", m_NoiseStrength);
	m_Shader->SetUniform1f("u_CrackShapeDistortion", m_CrackShapeDistortion);
	m_Shader->SetUniform1f("u_RandomHeights", m_RandomHeights);
	m_Shader->SetUniform1f("u_Smoothness", m_Smoothness);  
	m_Shader->SetUniform1i("u_Seed", m_Seed);
	m_Shader->SetUniform1i("u_SquareValue", m_SquareValue);
	m_Shader->SetUniform1i("u_AbsoluteValue", m_AbsoluteValue);
	m_Shader->SetUniform3f("u_Offset", m_Offset[0], m_Offset[1], m_Offset[2]);
	m_Shader->SetUniform2f("u_MinMaxHeight", m_MinMaxHeight[0], m_MinMaxHeight[1]);
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->SetUniform1i("u_UseSeedTexture", seedTexture != nullptr);
	if (seedTexture) m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	m_RequireUpdation = false;   
}

nlohmann::json BiomeBaseShape_Cracks::OnSave()
{
	nlohmann::json data;
	// data["m_SubStyle"] = m_SubStyle;
	return data;
}

void BiomeBaseShape_Cracks::OnLoad(nlohmann::json data)
{
	// if(data.contains("m_Height")) m_Height = data["m_Height"]; else m_Height = 0.5f;
	m_RequireUpdation = true;
}
