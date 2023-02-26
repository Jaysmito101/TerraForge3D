#include "Renderer/WireframeRenderer.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

WireframeRenderer::WireframeRenderer(ApplicationState* appState)
{
	m_AppState = appState;
	ReloadShaders();
}

WireframeRenderer::~WireframeRenderer()
{
	if (m_Shader) delete m_Shader;
}

void WireframeRenderer::Render(RendererViewport* viewport)
{
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glDepthMask(GL_TRUE); glDepthRange(0.0f, 1.0f);
	viewport->m_Camera.UpdateCamera();
	m_Shader->Bind();
	//glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Projection"), 1, GL_FALSE, glm::value_ptr(viewport->m_Camera.pers));
	//glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_View"), 1, GL_FALSE, glm::value_ptr(viewport->m_Camera.view));
	glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ProjectionView"), 1, GL_FALSE, glm::value_ptr(viewport->m_Camera.pv));
	glUniform3fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_CameraPosition"), 1, viewport->m_Camera.position);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Resolution"), m_AppState->mainMap.tileResolution);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_InvertNormals"), m_InvertNormals);
	glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileSize"), m_AppState->mainMap.tileSize);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SubTileSize"), m_AppState->workManager->GetWorkResolution());
	glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileOffset"), m_AppState->mainMap.tileOffsetX, m_AppState->mainMap.tileOffsetY);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	m_AppState->mainModel->Render();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void WireframeRenderer::ShowSettings()
{
	//ImGui::Checkbox("Invert Normals", &m_InvertNormals);
	if (ImGui::Button("Reload Shaders")) ReloadShaders();
}

void WireframeRenderer::ReloadShaders()
{
	if (m_Shader) delete m_Shader;
	bool success = false;
	m_Shader = new Shader(
		ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "wireframe_mode" PATH_SEPARATOR "vert.glsl", &success),
		ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "wireframe_mode" PATH_SEPARATOR "frag.glsl", &success)
	);
}
