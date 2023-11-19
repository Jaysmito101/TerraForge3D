#include "Renderer/ShadedRenderer.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

ShadedRenderer::ShadedRenderer(ApplicationState* appState)
{
	m_AppState = appState;
	ReloadShaders();
}

ShadedRenderer::~ShadedRenderer()
{
}

void ShadedRenderer::Render(RendererViewport* viewport)
{
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glDepthMask(GL_TRUE); glDepthRange(0.0f, 1.0f);
	viewport->m_Camera.UpdateCamera();
	m_Shader->Bind();
	m_AppState->generationManager->GetHeightmapData()->Bind(0);
	//glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Projection"), 1, GL_FALSE, glm::value_ptr(viewport->m_Camera.pers));
	//glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_View"), 1, GL_FALSE, glm::value_ptr(viewport->m_Camera.view));
	glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ProjectionView"), 1, GL_FALSE, glm::value_ptr(viewport->m_Camera.pv));
	glUniform3fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_CameraPosition"), 1, viewport->m_Camera.position);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Resolution"), m_AppState->mainMap.tileResolution);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_InvertNormals"), m_InvertNormals);
	glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileSize"), m_AppState->mainMap.tileSize);
	glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileOffset"), m_AppState->mainMap.tileOffsetX, m_AppState->mainMap.tileOffsetY);
	auto& rendererLights = m_AppState->rendererManager->GetRendererLights()->m_RendererLights; auto renderLightsCount = std::min((int)rendererLights.size(), OBJECT_RENDERER_MAX_LIGHTS);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_LightCount"), (int)renderLightsCount);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_EnableSkyLight"), (m_AppState->rendererManager->GetRendererLights()->m_UseSkyLight && m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady()) ? GL_TRUE : GL_FALSE);
	glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SkyLightIntensity"), (m_AppState->rendererManager->GetRendererLights()->m_SkyLightIntensity));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_AppState->rendererManager->GetSkyRenderer()->GetIrradianceMap());
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_IrradianceMap"), 1);
	for (int i = 0; i < rendererLights.size(); i++)
	{
		glUniform3f(glGetUniformLocation(m_Shader->GetNativeShader(), ("u_Lights[" + std::to_string(i) + "].position").c_str()), rendererLights[i].position.x, rendererLights[i].position.y, rendererLights[i].position.z);
		glUniform3f(glGetUniformLocation(m_Shader->GetNativeShader(), ("u_Lights[" + std::to_string(i) + "].color").c_str()), rendererLights[i].color.x, rendererLights[i].color.y, rendererLights[i].color.z);
		glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), ("u_Lights[" + std::to_string(i) + "].intensity").c_str()), rendererLights[i].intensity);
		glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), ("u_Lights[" + std::to_string(i) + "].type").c_str()), rendererLights[i].type);
	}

	m_AppState->mainModel->Render();
}

void ShadedRenderer::ShowSettings()
{
	ImGui::Checkbox("Invert Normals", &m_InvertNormals);
	if (ImGui::Button("Reload Shaders")) ReloadShaders();
}

void ShadedRenderer::ReloadShaders()
{
	m_Shader = m_AppState->resourceManager->LoadShader("pbr_mode", true);
}
