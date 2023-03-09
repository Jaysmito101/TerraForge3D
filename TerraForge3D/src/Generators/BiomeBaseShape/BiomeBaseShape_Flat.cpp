#include "Generators/BiomeBaseShape/BiomeBaseShape_Flat.h"
#include "Data/ApplicationState.h"

BiomeBaseShape_Flat::BiomeBaseShape_Flat(ApplicationState* appState)
	: BiomeBaseShapeGenerator(appState, "Basic") 
{
	s_TempBool = false;
	auto& source = ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "flat.glsl", &s_TempBool);
	if (!s_TempBool)
	{
		Log("Failed to load shader source file: %s" + m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "flat.glsl");
		return;
	}
	m_Shader = new ComputeShader(source);
}

BiomeBaseShape_Flat::~BiomeBaseShape_Flat()
{
	if (m_Shader) delete m_Shader;
}

bool BiomeBaseShape_Flat::ShowSettings()
{
	BASE_SHAPE_UI_PROPERTY(ImGui::DragFloat("Height", &m_Height, 0.01f));
	return m_RequireUpdation;
}

void BiomeBaseShape_Flat::Update(GeneratorData* buffer)
{
	auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	buffer->Bind(0);
	m_Shader->Bind();
	m_Shader->SetUniform1f("u_Height", m_Height);
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	m_RequireUpdation = false;
}

nlohmann::json BiomeBaseShape_Flat::OnSave()
{
	return nlohmann::json();
}

void BiomeBaseShape_Flat::OnLoad(nlohmann::json data)
{
}
