#include "Renderer/SeaRenderer.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Generators/GenerationManager.h"
#include "Generators/HeightfieldPyramid.h"
#include "Renderer/RendererLights.h"
#include "Renderer/RendererSky.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace tf3d::renderer
{

    SeaRenderer::SeaRenderer(ApplicationState *appState)
    {
        m_AppState = appState;
        glGenVertexArrays(1, &m_Vao);
        ReloadShaders();
    }

    SeaRenderer::~SeaRenderer()
    {
        if (m_Vao != 0)
            glDeleteVertexArrays(1, &m_Vao);
    }

    void SeaRenderer::BindUniforms(RendererViewport *viewport, float terrainWorldSize,
                                   float terrainHeightOffset, float seaWorldHeight,
                                   const glm::vec2 &surfaceMinimumXZ, const glm::vec2 &surfaceWorldSize)
    {
        const int shader                      = m_Shader->GetNativeShader();
        const glm::mat4 &projectionView       = viewport->m_Camera.GetProjectionViewMatrix();
        const glm::mat4 inverseProjection     = glm::inverse(viewport->m_Camera.GetProjectionMatrix());
        const glm::mat4 &view                 = viewport->m_Camera.GetViewMatrix();
        const glm::mat4 inverseProjectionView = glm::inverse(projectionView);
        const glm::vec3 &cameraPosition       = viewport->m_Camera.GetPosition();
        const glm::vec2 terrainMinimumXZ(-terrainWorldSize * 0.5f);

        glUniformMatrix4fv(glGetUniformLocation(shader, "u_ProjectionView"), 1, GL_FALSE,
                           glm::value_ptr(projectionView));
        glUniformMatrix4fv(glGetUniformLocation(shader, "u_InverseProjectionView"), 1, GL_FALSE,
                           glm::value_ptr(inverseProjectionView));
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
        glUniform1f(glGetUniformLocation(shader, "u_Time"), m_ElapsedTime);

        glUniform1f(glGetUniformLocation(shader, "u_WaveAmplitude"), m_Settings.waveAmplitude);
        glUniform1f(glGetUniformLocation(shader, "u_WaveLength"),
                    std::max(m_Settings.waveLength, 0.001f));
        glUniform1f(glGetUniformLocation(shader, "u_WaveSpeed"), m_Settings.waveSpeed);
        glUniform1f(glGetUniformLocation(shader, "u_WaveChoppiness"), m_Settings.waveChoppiness);
        glUniform1f(glGetUniformLocation(shader, "u_ShoreWidth"),
                    std::max(m_Settings.shoreWidth, 0.001f));
        glUniform1f(glGetUniformLocation(shader, "u_ShoreSoftness"),
                    std::clamp(m_Settings.shoreSoftness, 0.0f, 1.0f));
        glUniform1f(glGetUniformLocation(shader, "u_DeepDepth"),
                    std::max(m_Settings.deepDepth, 0.001f));
        glUniform1f(glGetUniformLocation(shader, "u_NormalStrength"), m_Settings.normalStrength);
        glUniform1f(glGetUniformLocation(shader, "u_NormalScale"),
                    std::max(m_Settings.normalScale, 0.01f));
        glUniform1f(glGetUniformLocation(shader, "u_RefractionStrength"), m_Settings.refractionStrength);
        glUniform1f(glGetUniformLocation(shader, "u_ReflectionStrength"), m_Settings.reflectionStrength);
        glUniform1f(glGetUniformLocation(shader, "u_Opacity"), m_Settings.opacity);
        glUniform1f(glGetUniformLocation(shader, "u_FoamStrength"), m_Settings.foamStrength);
        glUniform1f(glGetUniformLocation(shader, "u_NearshoreFoamWidth"),
                    std::clamp(m_Settings.nearshoreFoamWidth, 0.02f, 0.8f));
        glUniform1f(glGetUniformLocation(shader, "u_OffshoreFoamStrength"),
                    std::max(m_Settings.offshoreFoamStrength, 0.0f));
        glUniform1f(glGetUniformLocation(shader, "u_FoamScale"),
                    std::max(m_Settings.foamScale, 0.01f));
        glUniform1f(glGetUniformLocation(shader, "u_FoamSpeed"), m_Settings.foamSpeed);
        glUniform3fv(glGetUniformLocation(shader, "u_ShallowColor"), 1,
                     glm::value_ptr(m_Settings.shallowColor));
        glUniform3fv(glGetUniformLocation(shader, "u_DeepColor"), 1,
                     glm::value_ptr(m_Settings.deepColor));
        glUniform3fv(glGetUniformLocation(shader, "u_FoamColor"), 1,
                     glm::value_ptr(m_Settings.foamColor));

        const auto *heightPyramid = m_AppState->generationManager->GetHeightPyramid();
        glUniform1i(glGetUniformLocation(shader, "u_HeightPyramid"), 5);
        glUniform1i(glGetUniformLocation(shader, "u_PyramidLevels"),
                    heightPyramid != nullptr ? heightPyramid->GetMipLevels() : 1);
        glUniform1i(glGetUniformLocation(shader, "u_SceneColor"), 6);
        glUniform1i(glGetUniformLocation(shader, "u_SceneDepth"), 7);

        auto *rendererLights = m_AppState->rendererManager->GetRendererLights();
        auto *skyRenderer    = m_AppState->rendererManager->GetSkyRenderer();
        const bool skyReady  = rendererLights != nullptr && skyRenderer != nullptr &&
                              rendererLights->m_UseSkyLight && skyRenderer->IsSkyReady();
        const RendererSunData defaultSun{};
        const RendererSunData &sun = rendererLights != nullptr ? rendererLights->m_Sun : defaultSun;
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

    void SeaRenderer::Render(RendererViewport *viewport)
    {
        if (!m_Settings.enabled || viewport == nullptr || m_Shader == nullptr ||
            m_AppState == nullptr || m_AppState->generationManager == nullptr ||
            m_AppState->mainModel == nullptr || !m_AppState->mainModel->isGeneratedPlane ||
            m_AppState->generationManager->GetHeightPyramid() == nullptr ||
            !m_AppState->generationManager->GetHeightPyramid()->IsReady()) {
            return;
        }

        const float terrainWorldSize    = std::max(std::abs(m_AppState->mainMap.tileSize) * 2.0f, 0.0001f);
        const auto &fieldStatistics     = m_AppState->generationManager->GetFieldStatisticsResult();
        const float fieldMinimum        = fieldStatistics.valid ? fieldStatistics.minimum : 0.0f;
        const float solidDepth          = std::max(m_AppState->mainModel->planeSolidDepth, 0.0001f);
        const float terrainHeightOffset = -fieldMinimum + solidDepth;
        const float seaWorldHeight      = m_Settings.seaLevel + terrainHeightOffset;
        const glm::vec2 surfaceWorldSize(terrainWorldSize);
        const glm::vec2 surfaceMinimumXZ(-terrainWorldSize * 0.5f);
        m_ElapsedTime = static_cast<float>(glfwGetTime());

        viewport->m_FrameBuffer->Begin();
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);

        m_Shader->Bind();
        BindUniforms(viewport, terrainWorldSize, terrainHeightOffset, seaWorldHeight,
                     surfaceMinimumXZ, surfaceWorldSize);

        const auto *heightPyramid = m_AppState->generationManager->GetHeightPyramid();
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, heightPyramid->GetRendererID());
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, viewport->m_FrameBuffer->GetColorTexture());
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, viewport->m_FrameBuffer->GetResolvedDepthTexture());

        auto *skyRenderer   = m_AppState->rendererManager->GetSkyRenderer();
        const bool skyReady = skyRenderer != nullptr && skyRenderer->IsSkyReady();
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyReady ? skyRenderer->GetSpecularMap() : 0);

        glBindVertexArray(m_Vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        m_Shader->Unbind();
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);
    }

    void SeaRenderer::ShowSettings()
    {
        ImGui::Checkbox("Enable Sea", &m_Settings.enabled);
        ImGui::TextWrapped("Fullscreen GPU water volume using the heightfield pyramid and resolved scene depth.");

        ImGui::Separator();
        ImGui::TextUnformatted("Water Level");
        ImGui::DragFloat("Sea Level (field units)", &m_Settings.seaLevel, 0.01f, 0.0f, 0.0f, "%.4f");
        ImGui::DragFloat("Shore Width", &m_Settings.shoreWidth, 0.002f, 0.001f, 10.0f, "%.3f");
        ImGui::DragFloat("Shore Softness", &m_Settings.shoreSoftness, 0.01f, 0.0f, 1.0f, "%.3f");
        ImGui::DragFloat("Deep Water Depth", &m_Settings.deepDepth, 0.01f, 0.001f, 100.0f, "%.3f");

        ImGui::Separator();
        ImGui::TextUnformatted("Waves");
        ImGui::DragFloat("Wave Amplitude", &m_Settings.waveAmplitude, 0.001f, 0.0f, 10.0f, "%.4f");
        ImGui::DragFloat("Wave Length (terrain scale)", &m_Settings.waveLength, 0.01f, 0.01f, 2.0f, "%.3f");
        ImGui::DragFloat("Wave Speed", &m_Settings.waveSpeed, 0.01f, -10.0f, 10.0f, "%.3f");
        ImGui::DragFloat("Wave Choppiness", &m_Settings.waveChoppiness, 0.01f, 0.0f, 2.0f, "%.3f");

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
        ImGui::DragFloat("Nearshore Foam", &m_Settings.foamStrength, 0.01f, 0.0f, 2.0f, "%.3f");
        ImGui::DragFloat("Nearshore Crest Width", &m_Settings.nearshoreFoamWidth,
                         0.01f, 0.02f, 0.8f, "%.3f");
        ImGui::DragFloat("Offshore Foam", &m_Settings.offshoreFoamStrength,
                         0.01f, 0.0f, 2.0f, "%.3f");
        ImGui::DragFloat("Foam Scale (terrain UV)", &m_Settings.foamScale, 0.1f, 0.1f, 80.0f, "%.2f");
        ImGui::DragFloat("Foam Speed", &m_Settings.foamSpeed, 0.01f, -10.0f, 10.0f, "%.3f");
        ImGui::ColorEdit3("Foam Color", glm::value_ptr(m_Settings.foamColor));

        if (ImGui::Button("Reload Sea Shaders"))
            ReloadShaders();
    }

    void SeaRenderer::ReloadShaders()
    {
        if (m_AppState != nullptr && m_AppState->resourceManager != nullptr)
            m_Shader = m_AppState->resourceManager->LoadShader("sea", true);
    }
} // namespace tf3d::renderer