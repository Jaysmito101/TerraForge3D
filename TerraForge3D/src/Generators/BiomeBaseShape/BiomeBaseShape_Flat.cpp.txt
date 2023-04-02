#include "Generators/BiomeBaseShape/BiomeBaseShape_Flat.h"
#include "Data/ApplicationState.h"

BiomeBaseShape_Flat::BiomeBaseShape_Flat(ApplicationState* appState)
	: BiomeBaseShapeGenerator(appState, "Basic") 
{
	s_TempBool = false;
	auto path = m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "base_shape" PATH_SEPARATOR "flat.glsl";
	auto source = ReadShaderSourceFile(path, &s_TempBool);
	if (!s_TempBool)
	{
		Log("Failed to load shader source file: %s" + path);
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
	static const char* s_SubStyleNames[] = { "Flat", "Dome", "Slope", "Sine Wave"};
	static const char* s_SinWaveTypeNames[] = { "X", "Y", "X+Y", "X*Y"};
	BASE_SHAPE_UI_PROPERTY(ShowComboBox("Sub Style", &m_SubStyle, s_SubStyleNames, IM_ARRAYSIZE(s_SubStyleNames)));
	BASE_SHAPE_UI_PROPERTY(ImGui::DragFloat("Height", &m_Height, 0.01f));
	if(m_SubStyle == 3) BASE_SHAPE_UI_PROPERTY(ShowComboBox("Sin Wave Type", &m_SinWaveType, s_SinWaveTypeNames, IM_ARRAYSIZE(s_SinWaveTypeNames)));
	if(m_SubStyle == 1) BASE_SHAPE_UI_PROPERTY(ImGui::DragFloat("Radius", &m_Radius, 0.001f, 0.01f, 100.0f));
	if(m_SubStyle == 3) BASE_SHAPE_UI_PROPERTY(ImGui::DragFloat2("Frequency", m_Frequency, 0.01f));
	if(m_SubStyle == 3) BASE_SHAPE_UI_PROPERTY(ImGui::DragFloat2("Offset", m_Offset, 0.01f));
	if(m_SubStyle == 2 || m_SubStyle == 3) BASE_SHAPE_UI_PROPERTY(ImGui::DragFloat("Rotation", &m_Rotation, 0.01f));
	return m_RequireUpdation;
}

void BiomeBaseShape_Flat::Update(GeneratorData* buffer, GeneratorTexture* seedTexture)
{
	auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	buffer->Bind(0);
	m_Shader->Bind();
	m_Shader->SetUniform1f("u_Height", m_Height);
	m_Shader->SetUniform1f("u_Radius", m_Radius);
	m_Shader->SetUniform2f("u_Frequency", glm::vec2(m_Frequency[0], m_Frequency[1]));
	m_Shader->SetUniform2f("u_Offset", glm::vec2(m_Offset[0], m_Offset[1]));
	m_Shader->SetUniform1f("u_Rotation", m_Rotation);
	m_Shader->SetUniform1i("u_SubStyle", m_SubStyle);
	m_Shader->SetUniform1i("u_SinWaveType", m_SinWaveType);
	m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	m_Shader->SetUniform1i("u_UseSeedTexture", seedTexture != nullptr);
	if (seedTexture) m_Shader->SetUniform1i("u_SeedTexture", seedTexture->Bind(1));
	m_Shader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
	m_RequireUpdation = false;
}

nlohmann::json BiomeBaseShape_Flat::OnSave()
{
	nlohmann::json data;
	data["m_Height"] = m_Height;
	data["m_Radius"] = m_Radius;
	data["m_Frequency"] = { m_Frequency[0], m_Frequency[1] };
	data["m_Offset"] = { m_Offset[0], m_Offset[1] };
	data["m_Rotation"] = m_Rotation;
	data["m_SinWaveType"] = m_SinWaveType;
	data["m_SubStyle"] = m_SubStyle;
	return data;
}

void BiomeBaseShape_Flat::OnLoad(nlohmann::json data)
{
	if(data.contains("m_Height")) m_Height = data["m_Height"]; else m_Height = 0.5f;
	if(data.contains("m_Radius")) m_Radius = data["m_Radius"]; else m_Radius = 1.0f;
	if(data.contains("m_Frequency")) { m_Frequency[0] = data["m_Frequency"][0]; m_Frequency[1] = data["m_Frequency"][1]; } else { m_Frequency[0] = 1.0f; m_Frequency[1] = 1.0f; }
	if(data.contains("m_Offset")) { m_Offset[0] = data["m_Offset"][0]; m_Offset[1] = data["m_Offset"][1]; } else { m_Offset[0] = 0.0f; m_Offset[1] = 0.0f; }
	if(data.contains("m_Rotation")) m_Rotation = data["m_Rotation"]; else m_Rotation = 0.0f;
	if(data.contains("m_SinWaveType")) m_SinWaveType = data["m_SinWaveType"]; else m_SinWaveType = 0;
	if(data.contains("m_SubStyle")) m_SubStyle = data["m_SubStyle"]; else m_SubStyle = 0;
	m_RequireUpdation = true;
}
