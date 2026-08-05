#include "Renderer/ObjectRenderer.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

ObjectRenderer::ObjectRenderer(ApplicationState* appState)
{
	m_AppState = appState;
	m_SharedMemoryBuffer = std::make_shared<ShaderStorageBuffer>();
	m_SharedMemoryBuffer->SetData(nullptr, sizeof(float) * 4, true);
	glGenVertexArrays(1, &m_PostProcessVao);
	ReloadShaders();
}

ObjectRenderer::~ObjectRenderer()
{
	if (m_PostProcessVao != 0) glDeleteVertexArrays(1, &m_PostProcessVao);
}

void ObjectRenderer::Render(RendererViewport* viewport)
{
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glDepthMask(GL_TRUE); glDepthRange(0.0f, 1.0f);
	m_Shader->Bind();
	m_AppState->generationManager->GetHeightmapData()->Bind(0);
	m_SharedMemoryBuffer->Bind(1);
	//glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Projection"), 1, GL_FALSE, glm::value_ptr(viewport->m_Camera.pers));
	//glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_View"), 1, GL_FALSE, glm::value_ptr(viewport->m_Camera.view));
	glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ProjectionView"), 1, GL_FALSE, glm::value_ptr(viewport->m_Camera.GetProjectionViewMatrix()));
	glUniform3fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_CameraPosition"), 1, glm::value_ptr(viewport->m_Camera.GetPosition()));
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Resolution"), m_AppState->mainMap.tileResolution);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_InvertNormals"), m_InvertNormals);
	glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileSize"), m_AppState->mainMap.tileSize);
	glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileOffset"), m_AppState->mainMap.tileOffsetX, m_AppState->mainMap.tileOffsetY);
	const bool isPlane = m_AppState->mainModel != nullptr && m_AppState->mainModel->isGeneratedPlane;
	const auto& fieldStatistics = m_AppState->generationManager->GetFieldStatisticsResult();
	const float fieldMinimum = fieldStatistics.valid ? fieldStatistics.minimum : 0.0f;
	const float solidDepth = isPlane ? std::max(m_AppState->mainModel->planeSolidDepth, 0.0001f) : 0.0f;
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_PlaneMode"), isPlane ? 1 : 0);
	glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_FieldMinimum"), fieldMinimum);
	glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_HeightOffset"), isPlane ? -fieldMinimum + solidDepth : 0.0f);
	glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SolidDepth"), solidDepth);
	if (isPlane && m_PostProcessShader != nullptr && viewport->m_Camera.GetPosition().y > 0.0f)
	{
		m_PostProcessShader->Bind();
		const glm::mat4 inverseProjectionView = glm::inverse(viewport->m_Camera.GetProjectionViewMatrix());
		glUniformMatrix4fv(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_InverseProjectionView"), 1, GL_FALSE, glm::value_ptr(inverseProjectionView));
		glUniformMatrix4fv(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_ProjectionView"), 1, GL_FALSE, glm::value_ptr(viewport->m_Camera.GetProjectionViewMatrix()));
		glUniform3fv(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_CameraPosition"), 1, glm::value_ptr(viewport->m_Camera.GetPosition()));
		glUniform2f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_ViewportResolution"), viewport->m_Width, viewport->m_Height);
		glUniform3f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_Color"),
			67.0f / 255.0f, 88.0f / 255.0f, 114.0f / 255.0f);
		glBindVertexArray(m_PostProcessVao);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);

		m_Shader->Bind();
	}
	
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_IsViewportActive"), (viewport->m_MousePosition[0] >= 0.0f && viewport->m_MousePosition[1] >= 0.0f) ? 1 : 0);
	glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_MousePos"), viewport->m_MousePosition[0], viewport->m_MousePosition[1]);
	glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ViewportResolution"), viewport->m_Width, viewport->m_Height);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_RequiresDrawBrush"), m_DrawBrushSettings && m_DrawBrushSettings->m_ShowBrushCursor && viewport->m_IsHovered);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_DrawMask"), m_DrawBrushSettings && m_DrawBrushSettings->m_ShowMask && m_DrawBrushSettings->m_MaskTexture != -1);
	if (m_DrawBrushSettings)
	{
		glUniform4f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_BrushSettings0"), m_DrawBrushSettings->m_BrushPositionX, m_DrawBrushSettings->m_BrushPositionY, m_DrawBrushSettings->m_BrushSize, m_DrawBrushSettings->m_BrushFalloff);
		glUniform3f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_MaskColor"), m_DrawBrushSettings->m_MaskColor.x, m_DrawBrushSettings->m_MaskColor.y, m_DrawBrushSettings->m_MaskColor.z);
		glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_InvertMask"), m_DrawBrushSettings->m_InvertMask ? 1 : 0);

		if (m_DrawBrushSettings->m_MaskTexture != -1)
		{
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, m_DrawBrushSettings->m_MaskTexture);
			glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_MaskTexture"), 2);
		}
	}


	const auto& sun = m_AppState->rendererManager->GetRendererLights()->m_Sun;
	glUniform3f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SunDirection"), sun.direction.x, sun.direction.y, sun.direction.z);
	glUniform3f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SunColor"), sun.color.x, sun.color.y, sun.color.z);
	glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SunIntensity"), sun.intensity);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_EnableSkyLight"), (m_AppState->rendererManager->GetRendererLights()->m_UseSkyLight && m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady()) ? GL_TRUE : GL_FALSE);
	glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SkyLightIntensity"), m_AppState->rendererManager->GetRendererLights()->m_SkyLightIntensity);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady() ? m_AppState->rendererManager->GetSkyRenderer()->GetIrradianceMap() : 0);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_IrradianceMap"), 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady() ? m_AppState->rendererManager->GetSkyRenderer()->GetSkyboxMap() : 0);
	glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SkyboxMap"), 2);
	
	m_AppState->mainModel->Render();

	m_SharedMemoryBuffer->GetData(viewport->m_PosOnTerrain, sizeof(float) * 4);

	// m_CustomBaseShapeDrawSettings = nullptr;
}

void ObjectRenderer::ShowSettings()
{
	ImGui::Checkbox("Invert Normals", &m_InvertNormals);
	if (ImGui::Button("Reload Shaders")) ReloadShaders();
}

void ObjectRenderer::ReloadShaders()
{
	m_Shader = m_AppState->resourceManager->LoadShader("object_mode", true);
	m_PostProcessShader = m_AppState->resourceManager->LoadShader("post_process/base_surface", true);
}
