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
        m_Inspector.LoadConfig(appState, "Sea");
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
        const glm::mat4 &projectionView       = viewport->GetCamera().GetProjectionViewMatrix();
        const glm::mat4 inverseProjection     = glm::inverse(viewport->GetCamera().GetProjectionMatrix());
        const glm::mat4 &view                 = viewport->GetCamera().GetViewMatrix();
        const glm::mat4 inverseProjectionView = glm::inverse(projectionView);
        const glm::vec3 &cameraPosition       = viewport->GetCamera().GetPosition();
        const glm::vec2 terrainMinimumXZ(-terrainWorldSize * 0.5f);
        m_Shader->SetUniformMat4("u_ProjectionView", projectionView);
        m_Shader->SetUniformMat4("u_InverseProjectionView", inverseProjectionView);
        m_Shader->SetUniformMat4("u_View", view);
        m_Shader->SetUniformMat4("u_InverseProjection", inverseProjection);
        m_Shader->SetUniform1i("u_Perspective", viewport->GetCamera().IsPerspective() ? 1 : 0);
        m_Shader->SetUniform3f("u_CameraPosition", cameraPosition);
        m_Shader->SetUniform2f("u_ViewportResolution", static_cast<float>(viewport->GetWidth()),
                               static_cast<float>(viewport->GetHeight()));
        m_Shader->SetUniform2f("u_TerrainMinimumXZ", terrainMinimumXZ);
        m_Shader->SetUniform2f("u_TerrainWorldSize", terrainWorldSize, terrainWorldSize);
        m_Shader->SetUniform2f("u_SurfaceMinimumXZ", surfaceMinimumXZ);
        m_Shader->SetUniform2f("u_SurfaceWorldSize", surfaceWorldSize);
        m_Shader->SetUniform1f("u_SeaWorldHeight", seaWorldHeight);
        m_Shader->SetUniform1f("u_TerrainHeightOffset", terrainHeightOffset);
        m_Shader->SetUniform1f("u_BottomWorldHeight", 0.0f);
        m_Shader->SetUniform1f("u_SideEdgeOffset", std::max(terrainWorldSize * 0.00025f, 0.00005f));
        m_Shader->SetUniform1f("u_Time", m_ElapsedTime);

        m_Inspector.ApplyToShader(*m_Shader);

        const auto *heightPyramid = m_AppState->generationManager->GetHeightPyramid();
        m_Shader->SetUniform1i("u_HeightPyramid", 5);
        m_Shader->SetUniform1i("u_PyramidLevels",
                               heightPyramid != nullptr ? heightPyramid->GetMipLevels() : 1);
        m_Shader->SetUniform1i("u_SceneColor", 6);
        m_Shader->SetUniform1i("u_SceneDepth", 7);

        auto *rendererLights = m_AppState->rendererManager->GetRendererLights();
        auto *skyRenderer    = m_AppState->rendererManager->GetSkyRenderer();
        const bool skyReady  = rendererLights != nullptr && skyRenderer != nullptr &&
                              rendererLights->IsSkyLightEnabled() && skyRenderer->IsSkyReady();
        m_Shader->SetUniform1i("u_EnableSkyLight", skyReady ? 1 : 0);
        m_Shader->SetUniform1f("u_SkyLightIntensity",
                               rendererLights != nullptr ? rendererLights->GetSkyLightIntensity() : 0.0f);
        m_Shader->SetUniform3f("u_SunDirection",
                               rendererLights != nullptr ? rendererLights->GetSunDirection() : glm::vec3(0.0f));
        m_Shader->SetUniform3f("u_SunColor",
                               rendererLights != nullptr ? rendererLights->GetSunColor() : glm::vec3(0.0f));
        m_Shader->SetUniform1f("u_SunIntensity",
                               rendererLights != nullptr ? rendererLights->GetSunIntensity() : 0.0f);
        m_Shader->SetUniform1i("u_SpecularMap", 2);
    }

    void SeaRenderer::Render(RendererViewport *viewport)
    {
        if (!IsEnabled() || viewport == nullptr || m_Shader == nullptr ||
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
        const float seaWorldHeight      = m_Inspector.Get<float>("SeaLevel") + terrainHeightOffset;
        const glm::vec2 surfaceWorldSize(terrainWorldSize);
        const glm::vec2 surfaceMinimumXZ(-terrainWorldSize * 0.5f);
        m_ElapsedTime = static_cast<float>(glfwGetTime());

        viewport->GetFrameBuffer()->Begin();
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
        glBindTexture(GL_TEXTURE_2D, viewport->GetFrameBuffer()->GetColorTexture());
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, viewport->GetFrameBuffer()->GetResolvedDepthTexture());

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
        m_Inspector.Render();
        // if (m_Inspector.GetLastAction() == "ReloadSeaShaders")
        ReloadShaders();
    }

    void SeaRenderer::ReloadShaders()
    {
        if (m_AppState != nullptr && m_AppState->resourceManager != nullptr)
            m_Shader = m_AppState->resourceManager->LoadShader("sea", true);
    }
} // namespace tf3d::renderer
