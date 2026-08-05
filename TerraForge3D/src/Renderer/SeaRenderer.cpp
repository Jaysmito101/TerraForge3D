#include "Renderer/SeaRenderer.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/GenerationManager.h"
#include "Generators/HeightfieldPyramid.h"
#include "Renderer/RendererLights.h"
#include "Renderer/RendererSky.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace
{
	struct SeaVertex
	{
		glm::vec4 position;
		glm::vec4 normal;
		glm::vec4 texCoord;
	};
}

SeaRenderer::SeaRenderer(ApplicationState* appState)
{
	m_AppState = appState;
	RebuildGrid(m_Settings.gridResolution);
	BuildSideGeometry();
	ReloadShaders();
}

SeaRenderer::~SeaRenderer()
{
	if (m_Ebo != 0) glDeleteBuffers(1, &m_Ebo);
	if (m_Vbo != 0) glDeleteBuffers(1, &m_Vbo);
	if (m_Vao != 0) glDeleteVertexArrays(1, &m_Vao);
	if (m_SideEbo != 0) glDeleteBuffers(1, &m_SideEbo);
	if (m_SideVbo != 0) glDeleteBuffers(1, &m_SideVbo);
	if (m_SideVao != 0) glDeleteVertexArrays(1, &m_SideVao);
}

void SeaRenderer::RebuildGrid(int32_t resolution)
{
	resolution = std::clamp(resolution, 16, 256);
	if (m_Vao != 0 && m_AllocatedGridResolution == resolution) return;

	if (m_Vao == 0) glGenVertexArrays(1, &m_Vao);
	if (m_Vbo == 0) glGenBuffers(1, &m_Vbo);
	if (m_Ebo == 0) glGenBuffers(1, &m_Ebo);

	std::vector<SeaVertex> vertices;
	vertices.resize(static_cast<size_t>(resolution) * static_cast<size_t>(resolution));
	for (int32_t y = 0; y < resolution; ++y)
	{
		for (int32_t x = 0; x < resolution; ++x)
		{
			const float u = static_cast<float>(x) / static_cast<float>(resolution - 1);
			const float v = static_cast<float>(y) / static_cast<float>(resolution - 1);
			SeaVertex& vertex = vertices[static_cast<size_t>(x + y * resolution)];
			vertex.position = glm::vec4(u, 0.0f, v, 1.0f);
			vertex.normal = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
			vertex.texCoord = glm::vec4(u, v, 0.0f, 0.0f);
		}
	}

	std::vector<uint32_t> indices;
	indices.reserve(static_cast<size_t>(resolution - 1) * static_cast<size_t>(resolution - 1) * 6u);
	for (int32_t y = 0; y < resolution - 1; ++y)
	{
		for (int32_t x = 0; x < resolution - 1; ++x)
		{
			const uint32_t index = static_cast<uint32_t>(x + y * resolution);
			indices.push_back(index);
			indices.push_back(index + static_cast<uint32_t>(resolution) + 1u);
			indices.push_back(index + static_cast<uint32_t>(resolution));
			indices.push_back(index);
			indices.push_back(index + 1u);
			indices.push_back(index + static_cast<uint32_t>(resolution) + 1u);
		}
	}

	glBindVertexArray(m_Vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
	glBufferData(GL_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(vertices.size() * sizeof(SeaVertex)),
		vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
		indices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(SeaVertex),
		reinterpret_cast<void*>(offsetof(SeaVertex, position)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SeaVertex),
		reinterpret_cast<void*>(offsetof(SeaVertex, normal)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SeaVertex),
		reinterpret_cast<void*>(offsetof(SeaVertex, texCoord)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	m_GridIndexCount = static_cast<int32_t>(indices.size());
	m_AllocatedGridResolution = resolution;
}

void SeaRenderer::BuildSideGeometry()
{
	if (m_SideVao == 0) glGenVertexArrays(1, &m_SideVao);
	if (m_SideVbo == 0) glGenBuffers(1, &m_SideVbo);
	if (m_SideEbo == 0) glGenBuffers(1, &m_SideEbo);

	std::vector<SeaVertex> vertices;
	std::vector<uint32_t> indices;
	auto addSide = [&](const glm::vec3& normal, const glm::vec3& bottom0,
		const glm::vec3& bottom1, const glm::vec2& uv0, const glm::vec2& uv1)
	{
		const uint32_t base = static_cast<uint32_t>(vertices.size());
		SeaVertex bottomVertex0{};
		bottomVertex0.position = glm::vec4(bottom0, 1.0f);
		bottomVertex0.normal = glm::vec4(normal, 0.0f);
		bottomVertex0.texCoord = glm::vec4(uv0, 0.0f, 0.0f);
		SeaVertex bottomVertex1 = bottomVertex0;
		bottomVertex1.position = glm::vec4(bottom1, 1.0f);
		bottomVertex1.texCoord = glm::vec4(uv1, 0.0f, 0.0f);
		SeaVertex topVertex0 = bottomVertex0;
		topVertex0.position.y = 1.0f;
		SeaVertex topVertex1 = bottomVertex1;
		topVertex1.position.y = 1.0f;
		vertices.push_back(bottomVertex0);
		vertices.push_back(bottomVertex1);
		vertices.push_back(topVertex0);
		vertices.push_back(topVertex1);
		indices.insert(indices.end(), { base, base + 1u, base + 2u,
			base + 2u, base + 1u, base + 3u });
	};

	addSide(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f));
	addSide(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f));
	addSide(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f));
	addSide(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f));

	glBindVertexArray(m_SideVao);
	glBindBuffer(GL_ARRAY_BUFFER, m_SideVbo);
	glBufferData(GL_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(vertices.size() * sizeof(SeaVertex)),
		vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SideEbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
		indices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(SeaVertex),
		reinterpret_cast<void*>(offsetof(SeaVertex, position)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SeaVertex),
		reinterpret_cast<void*>(offsetof(SeaVertex, normal)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SeaVertex),
		reinterpret_cast<void*>(offsetof(SeaVertex, texCoord)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	m_SideIndexCount = static_cast<int32_t>(indices.size());
}

void SeaRenderer::BindUniforms(RendererViewport* viewport, float terrainWorldSize,
	float terrainHeightOffset, float seaWorldHeight,
	const glm::vec2& surfaceMinimumXZ, const glm::vec2& surfaceWorldSize)
{
	const int shader = m_Shader->GetNativeShader();
	const glm::mat4& projectionView = viewport->m_Camera.GetProjectionViewMatrix();
	const glm::mat4 inverseProjection = glm::inverse(viewport->m_Camera.GetProjectionMatrix());
	const glm::mat4& view = viewport->m_Camera.GetViewMatrix();
	const glm::vec3& cameraPosition = viewport->m_Camera.GetPosition();
	const glm::vec2 terrainMinimumXZ(-terrainWorldSize * 0.5f);

	glUniformMatrix4fv(glGetUniformLocation(shader, "u_ProjectionView"), 1, GL_FALSE,
		glm::value_ptr(projectionView));
	glUniformMatrix4fv(glGetUniformLocation(shader, "u_View"), 1, GL_FALSE,
		glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(shader, "u_InverseProjection"), 1, GL_FALSE,
		glm::value_ptr(inverseProjection));
	glUniform1i(glGetUniformLocation(shader, "u_Perspective"),
		viewport->m_Camera.IsPerspective() ? 1 : 0);
	glUniform3fv(glGetUniformLocation(shader, "u_CameraPosition"), 1,
		glm::value_ptr(cameraPosition));
	glUniform2f(glGetUniformLocation(shader, "u_ViewportResolution"),
		static_cast<float>(viewport->m_Width), static_cast<float>(viewport->m_Height));
	glUniform2f(glGetUniformLocation(shader, "u_TerrainMinimumXZ"),
		terrainMinimumXZ.x, terrainMinimumXZ.y);
	glUniform2f(glGetUniformLocation(shader, "u_TerrainWorldSize"),
		terrainWorldSize, terrainWorldSize);
	glUniform2f(glGetUniformLocation(shader, "u_SurfaceMinimumXZ"),
		surfaceMinimumXZ.x, surfaceMinimumXZ.y);
	glUniform2f(glGetUniformLocation(shader, "u_SurfaceWorldSize"),
		surfaceWorldSize.x, surfaceWorldSize.y);
	glUniform1f(glGetUniformLocation(shader, "u_SeaLevel"), m_Settings.seaLevel);
	glUniform1f(glGetUniformLocation(shader, "u_SeaWorldHeight"), seaWorldHeight);
	glUniform1f(glGetUniformLocation(shader, "u_TerrainHeightOffset"), terrainHeightOffset);
	glUniform1f(glGetUniformLocation(shader, "u_BottomWorldHeight"), 0.0f);
	glUniform1f(glGetUniformLocation(shader, "u_SideEdgeOffset"),
		std::max(terrainWorldSize * 0.00025f, 0.00005f));
	glUniform1i(glGetUniformLocation(shader, "u_VerticalSidePass"), 0);
	glUniform1f(glGetUniformLocation(shader, "u_Time"), m_ElapsedTime);

	glUniform1f(glGetUniformLocation(shader, "u_WaveAmplitude"), m_Settings.waveAmplitude);
	glUniform1f(glGetUniformLocation(shader, "u_WaveLength"), std::max(m_Settings.waveLength, 0.001f));
	glUniform1f(glGetUniformLocation(shader, "u_WaveSpeed"), m_Settings.waveSpeed);
	glUniform1f(glGetUniformLocation(shader, "u_WaveChoppiness"), m_Settings.waveChoppiness);
	glUniform1f(glGetUniformLocation(shader, "u_ShoreWidth"), std::max(m_Settings.shoreWidth, 0.001f));
	glUniform1f(glGetUniformLocation(shader, "u_DeepDepth"), std::max(m_Settings.deepDepth, 0.001f));
	glUniform1f(glGetUniformLocation(shader, "u_NormalStrength"), m_Settings.normalStrength);
	glUniform1f(glGetUniformLocation(shader, "u_NormalScale"), std::max(m_Settings.normalScale, 0.01f));
	glUniform1f(glGetUniformLocation(shader, "u_RefractionStrength"), m_Settings.refractionStrength);
	glUniform1f(glGetUniformLocation(shader, "u_ReflectionStrength"), m_Settings.reflectionStrength);
	glUniform1f(glGetUniformLocation(shader, "u_Opacity"), m_Settings.opacity);
	glUniform1f(glGetUniformLocation(shader, "u_FoamStrength"), m_Settings.foamStrength);
	glUniform1f(glGetUniformLocation(shader, "u_FoamScale"), std::max(m_Settings.foamScale, 0.01f));
	glUniform1f(glGetUniformLocation(shader, "u_FoamSpeed"), m_Settings.foamSpeed);
	glUniform3fv(glGetUniformLocation(shader, "u_ShallowColor"), 1,
		glm::value_ptr(m_Settings.shallowColor));
	glUniform3fv(glGetUniformLocation(shader, "u_DeepColor"), 1,
		glm::value_ptr(m_Settings.deepColor));
	glUniform3fv(glGetUniformLocation(shader, "u_FoamColor"), 1,
		glm::value_ptr(m_Settings.foamColor));

	const auto* heightPyramid = m_AppState->generationManager->GetHeightPyramid();
	glUniform1i(glGetUniformLocation(shader, "u_HeightPyramid"), 5);
	glUniform1i(glGetUniformLocation(shader, "u_PyramidLevels"),
		heightPyramid != nullptr ? heightPyramid->GetMipLevels() : 1);
	glUniform1i(glGetUniformLocation(shader, "u_SceneColor"), 6);
	glUniform1i(glGetUniformLocation(shader, "u_SceneDepth"), 7);

	auto* rendererLights = m_AppState->rendererManager->GetRendererLights();
	auto* skyRenderer = m_AppState->rendererManager->GetSkyRenderer();
	const bool skyReady = rendererLights != nullptr && skyRenderer != nullptr &&
		rendererLights->m_UseSkyLight && skyRenderer->IsSkyReady();
	const RendererSunData defaultSun{};
	const RendererSunData& sun = rendererLights != nullptr ? rendererLights->m_Sun : defaultSun;
	glUniform1i(glGetUniformLocation(shader, "u_EnableSkyLight"), skyReady ? 1 : 0);
	glUniform1f(glGetUniformLocation(shader, "u_SkyLightIntensity"),
		rendererLights != nullptr ? rendererLights->m_SkyLightIntensity : 0.0f);
	glUniform3fv(glGetUniformLocation(shader, "u_SunDirection"), 1,
		glm::value_ptr(sun.direction));
	glUniform3fv(glGetUniformLocation(shader, "u_SunColor"), 1,
		glm::value_ptr(sun.color));
	glUniform1f(glGetUniformLocation(shader, "u_SunIntensity"), sun.intensity);
	glUniform1i(glGetUniformLocation(shader, "u_SpecularMap"), 2);
}

void SeaRenderer::Render(RendererViewport* viewport)
{
	if (!m_Settings.enabled || viewport == nullptr || m_Shader == nullptr ||
		m_AppState == nullptr || m_AppState->generationManager == nullptr ||
		m_AppState->mainModel == nullptr || !m_AppState->mainModel->isGeneratedPlane ||
		m_AppState->generationManager->GetHeightPyramid() == nullptr ||
		!m_AppState->generationManager->GetHeightPyramid()->IsReady())
	{
		return;
	}

	if (m_AllocatedGridResolution != std::clamp(m_Settings.gridResolution, 16, 256))
		RebuildGrid(m_Settings.gridResolution);

	const float terrainWorldSize = std::max(std::abs(m_AppState->mainMap.tileSize) * 2.0f, 0.0001f);
	const auto& fieldStatistics = m_AppState->generationManager->GetFieldStatisticsResult();
	const float fieldMinimum = fieldStatistics.valid ? fieldStatistics.minimum : 0.0f;
	const float solidDepth = std::max(m_AppState->mainModel->planeSolidDepth, 0.0001f);
	const float terrainHeightOffset = -fieldMinimum + solidDepth;
	const float seaWorldHeight = m_Settings.seaLevel + terrainHeightOffset;
	const float surfaceWorldSizeValue = terrainWorldSize;
	const glm::vec2 surfaceWorldSize(surfaceWorldSizeValue);
	const glm::vec2 surfaceMinimumXZ(-surfaceWorldSizeValue * 0.5f);
	const float bottomWorldHeight = terrainHeightOffset + fieldMinimum - solidDepth;
	m_ElapsedTime = static_cast<float>(glfwGetTime());

	viewport->m_FrameBuffer->Begin();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	m_Shader->Bind();
	BindUniforms(viewport, terrainWorldSize, terrainHeightOffset, seaWorldHeight,
		surfaceMinimumXZ, surfaceWorldSize);

	const auto* heightPyramid = m_AppState->generationManager->GetHeightPyramid();
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, heightPyramid->GetRendererID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, viewport->m_FrameBuffer->GetColorTexture());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, viewport->m_FrameBuffer->GetResolvedDepthTexture());

	auto* skyRenderer = m_AppState->rendererManager->GetSkyRenderer();
	const bool skyReady = skyRenderer != nullptr && skyRenderer->IsSkyReady();
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyReady ? skyRenderer->GetSpecularMap() : 0);

	glBindVertexArray(m_Vao);
	glDrawElements(GL_TRIANGLES, m_GridIndexCount, GL_UNSIGNED_INT, nullptr);

	if (seaWorldHeight > bottomWorldHeight + 0.0001f && m_SideIndexCount > 0)
	{
		const int shader = m_Shader->GetNativeShader();
		glUniform1f(glGetUniformLocation(shader, "u_BottomWorldHeight"), bottomWorldHeight);
		glUniform1i(glGetUniformLocation(shader, "u_VerticalSidePass"), 1);
		glBindVertexArray(m_SideVao);
		glDrawElements(GL_TRIANGLES, m_SideIndexCount, GL_UNSIGNED_INT, nullptr);
	}
	glBindVertexArray(0);

	m_Shader->Unbind();
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);
}

void SeaRenderer::ShowSettings()
{
	ImGui::Checkbox("Enable Sea", &m_Settings.enabled);
	ImGui::TextWrapped("Uses the GPU heightfield mipmap for depth, shorelines, wave attenuation, and foam.");

	ImGui::Separator();
	ImGui::TextUnformatted("Water Level");
	ImGui::DragFloat("Sea Level (field units)", &m_Settings.seaLevel, 0.01f, 0.0f, 0.0f, "%.4f");
	ImGui::DragFloat("Shore Width", &m_Settings.shoreWidth, 0.002f, 0.001f, 10.0f, "%.3f");
	ImGui::DragFloat("Deep Water Depth", &m_Settings.deepDepth, 0.01f, 0.001f, 100.0f, "%.3f");

	ImGui::Separator();
	ImGui::TextUnformatted("Waves");
	ImGui::DragFloat("Wave Amplitude", &m_Settings.waveAmplitude, 0.001f, 0.0f, 10.0f, "%.4f");
	ImGui::DragFloat("Wave Length (terrain scale)", &m_Settings.waveLength, 0.01f, 0.01f, 2.0f, "%.3f");
	ImGui::DragFloat("Wave Speed", &m_Settings.waveSpeed, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::DragFloat("Wave Choppiness", &m_Settings.waveChoppiness, 0.01f, 0.0f, 2.0f, "%.3f");
	ImGui::SliderInt("Surface Grid", &m_Settings.gridResolution, 32, 192);

	ImGui::Separator();
	ImGui::TextUnformatted("Surface Shading");
	ImGui::DragFloat("Normal Strength", &m_Settings.normalStrength, 0.01f, 0.0f, 2.0f, "%.3f");
	ImGui::DragFloat("Normal Scale (terrain UV)", &m_Settings.normalScale, 0.05f, 0.05f, 40.0f, "%.2f");
	ImGui::DragFloat("Refraction", &m_Settings.refractionStrength, 0.001f, 0.0f, 0.25f, "%.4f");
	ImGui::DragFloat("Reflection", &m_Settings.reflectionStrength, 0.01f, 0.0f, 2.0f, "%.3f");
	ImGui::DragFloat("Opacity", &m_Settings.opacity, 0.01f, 0.0f, 1.0f, "%.3f");
	ImGui::ColorEdit3("Shallow Color", glm::value_ptr(m_Settings.shallowColor));
	ImGui::ColorEdit3("Deep Color", glm::value_ptr(m_Settings.deepColor));

	ImGui::Separator();
	ImGui::TextUnformatted("Foam");
	ImGui::DragFloat("Foam Strength", &m_Settings.foamStrength, 0.01f, 0.0f, 2.0f, "%.3f");
	ImGui::DragFloat("Foam Scale (terrain UV)", &m_Settings.foamScale, 0.1f, 0.1f, 80.0f, "%.2f");
	ImGui::DragFloat("Foam Speed", &m_Settings.foamSpeed, 0.01f, -10.0f, 10.0f, "%.3f");
	ImGui::ColorEdit3("Foam Color", glm::value_ptr(m_Settings.foamColor));

	if (ImGui::Button("Reload Sea Shaders")) ReloadShaders();
}

void SeaRenderer::ReloadShaders()
{
	if (m_AppState != nullptr && m_AppState->resourceManager != nullptr)
		m_Shader = m_AppState->resourceManager->LoadShader("sea", true);
}
