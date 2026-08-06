#include "Renderer/ObjectRenderer.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace tf3d::renderer
{

    ObjectRenderer::ObjectRenderer(ApplicationState *appState)
    {
        m_AppState           = appState;
        m_SharedMemoryBuffer = std::make_shared<ShaderStorageBuffer>();
        m_SharedMemoryBuffer->SetData(nullptr, sizeof(float) * 4, true);
        glGenVertexArrays(1, &m_PostProcessVao);
        ReloadShaders();
    }

    ObjectRenderer::~ObjectRenderer()
    {
        if (m_PostProcessVao != 0)
            glDeleteVertexArrays(1, &m_PostProcessVao);
    }

    void ObjectRenderer::Render(RendererViewport *viewport)
    {
        TF3D_PROFILE_SCOPE("renderer/object");
        TF3D_PROFILE_BEGIN(objectSetupProfile, "renderer/object/setup");
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDepthRange(0.0f, 1.0f);
        m_Shader->Bind();
        m_AppState->generationManager->GetHeightmapData()->Bind(0);
        m_SharedMemoryBuffer->Bind(1);
        auto *slopeTexture         = m_AppState->generationManager->GetSlopeTexture();
        const bool hasSlopeTexture = m_AppState->generationManager->HasSlopeTexture();
        if (slopeTexture && hasSlopeTexture) {
            slopeTexture->Bind(5);
        } else {
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        auto *terrainSelfShadow         = m_AppState->rendererManager->GetTerrainSelfShadow();
        const bool hasTerrainSelfShadow = terrainSelfShadow != nullptr && terrainSelfShadow->IsReady();
        if (hasTerrainSelfShadow) {
            terrainSelfShadow->Bind(6);
        } else {
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        m_Shader->SetUniform1i("u_TerrainSelfShadow", 6);
        m_Shader->SetUniform1i("u_HasTerrainSelfShadow", hasTerrainSelfShadow ? 1 : 0);
        auto *heightfieldAmbient         = m_AppState->rendererManager->GetHeightfieldAmbientCache();
        const bool hasHeightfieldAmbient = heightfieldAmbient != nullptr && heightfieldAmbient->IsReady();
        if (hasHeightfieldAmbient) {
            heightfieldAmbient->Bind(8);
        } else {
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        m_Shader->SetUniform1i("u_TerrainAmbient", 8);
        m_Shader->SetUniform1i("u_HasTerrainAmbient", hasHeightfieldAmbient ? 1 : 0);
        auto *heightfieldGI         = m_AppState->rendererManager->GetHeightfieldGICache();
        const bool hasHeightfieldGI = heightfieldGI != nullptr && heightfieldGI->IsReady();
        if (hasHeightfieldGI) {
            heightfieldGI->Bind(9);
        } else {
            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        m_Shader->SetUniform1i("u_TerrainGI", 9);
        m_Shader->SetUniform1i("u_HasTerrainGI", hasHeightfieldGI ? 1 : 0);
        m_Shader->SetUniform1i("u_SlopeTexture", 5);
        m_Shader->SetUniform1i("u_HasSlopeTexture", hasSlopeTexture ? 1 : 0);
        m_Shader->SetUniformMat4("u_ProjectionView", viewport->GetCamera().GetProjectionViewMatrix());
        m_Shader->SetUniform3f("u_CameraPosition", viewport->GetCamera().GetPosition());
        m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
        m_Shader->SetUniform1i("u_InvertNormals", m_InvertNormals);
        m_Shader->SetUniform1i("u_ViewNormals", m_ViewNormals ? 1 : 0);
        m_Shader->SetUniform1i("u_ViewSlope", m_ViewSlope ? 1 : 0);
        m_Shader->SetUniform1i("u_ViewTerrainSelfShadow", m_ViewTerrainSelfShadow ? 1 : 0);
        m_Shader->SetUniform1i("u_ViewTerrainAmbient", m_ViewTerrainAmbient ? 1 : 0);
        m_Shader->SetUniform1i("u_ViewTerrainBentNormal", m_ViewTerrainBentNormal ? 1 : 0);
        m_Shader->SetUniform1f("u_TileSize", m_AppState->mainMap.tileSize);
        m_Shader->SetUniform2f("u_TileOffset", m_AppState->mainMap.tileOffsetX,
                               m_AppState->mainMap.tileOffsetY);
        const bool isPlane          = m_AppState->mainModel != nullptr && m_AppState->mainModel->isGeneratedPlane;
        const auto &fieldStatistics = m_AppState->generationManager->GetFieldStatisticsResult();
        const float fieldMinimum    = fieldStatistics.valid ? fieldStatistics.minimum : 0.0f;
        const float solidDepth      = isPlane ? std::max(m_AppState->mainModel->planeSolidDepth, 0.0001f) : 0.0f;
        m_Shader->SetUniform1i("u_PlaneMode", isPlane ? 1 : 0);
        m_Shader->SetUniform1f("u_FieldMinimum", fieldMinimum);
        m_Shader->SetUniform1f("u_HeightOffset", isPlane ? -fieldMinimum + solidDepth : 0.0f);
        m_Shader->SetUniform1f("u_SolidDepth", solidDepth);
        TF3D_PROFILE_END(objectSetupProfile);
        {
            TF3D_PROFILE_SCOPE("renderer/object/post-process");
            if (isPlane && m_PostProcessShader != nullptr && viewport->GetCamera().GetPosition().y > 0.0f) {
                m_PostProcessShader->Bind();
                const glm::mat4 inverseProjectionView = glm::inverse(viewport->GetCamera().GetProjectionViewMatrix());
                m_PostProcessShader->SetUniformMat4("u_InverseProjectionView", inverseProjectionView);
                m_PostProcessShader->SetUniformMat4("u_ProjectionView", viewport->GetCamera().GetProjectionViewMatrix());
                m_PostProcessShader->SetUniform3f("u_CameraPosition", viewport->GetCamera().GetPosition());
                m_PostProcessShader->SetUniform2f("u_ViewportResolution", viewport->GetWidth(), viewport->GetHeight());
                m_PostProcessShader->SetUniform3f("u_Color", 67.0f / 255.0f,
                                                  88.0f / 255.0f, 114.0f / 255.0f);
                auto *rendererLights      = m_AppState->rendererManager->GetRendererLights();
                auto *skyRenderer         = m_AppState->rendererManager->GetSkyRenderer();
                const bool enableSkyLight = rendererLights->IsSkyLightEnabled() && skyRenderer->IsSkyReady();
                m_PostProcessShader->SetUniform1i("u_EnableSkyLight", enableSkyLight ? 1 : 0);
                m_PostProcessShader->SetUniform1f("u_SkyLightIntensity", rendererLights->GetSkyLightIntensity());
                m_PostProcessShader->SetUniform3f("u_SunDirection", rendererLights->GetSunDirection());
                m_PostProcessShader->SetUniform3f("u_SunColor", rendererLights->GetSunColor());
                m_PostProcessShader->SetUniform1f("u_SunIntensity", rendererLights->GetSunIntensity());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, enableSkyLight ? skyRenderer->GetIrradianceMap() : 0);
                m_PostProcessShader->SetUniform1i("u_IrradianceMap", 1);
                auto *planarShadowCache    = m_AppState->rendererManager->GetPlanarShadowCache();
                const bool hasPlanarShadow = planarShadowCache != nullptr && planarShadowCache->IsReady();
                if (hasPlanarShadow) {
                    planarShadowCache->Bind(7);
                } else {
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                m_PostProcessShader->SetUniform1i("u_TerrainPlanarShadow", 7);
                m_PostProcessShader->SetUniform1i("u_HasTerrainPlanarShadow", hasPlanarShadow ? 1 : 0);
                m_PostProcessShader->SetUniform1f("u_PlanarShadowSoftness",
                                                  m_AppState->rendererManager->GetPlanarShadowSoftness());
                if (hasPlanarShadow) {
                    const auto atlasMinimum   = planarShadowCache->GetAtlasMinimumXZ();
                    const auto atlasWorldSize = planarShadowCache->GetAtlasWorldSize();
                    m_PostProcessShader->SetUniform2f("u_PlanarShadowMinimumXZ", atlasMinimum);
                    m_PostProcessShader->SetUniform2f("u_PlanarShadowWorldSize", atlasWorldSize);
                }
                glBindVertexArray(m_PostProcessVao);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glBindVertexArray(0);

                m_Shader->Bind();
            }
        }

        TF3D_PROFILE_BEGIN(objectMaterialStateProfile, "renderer/object/material-state");
        const auto &mousePosition = viewport->GetMousePosition();
        m_Shader->SetUniform1i("u_IsViewportActive", (mousePosition[0] >= 0.0f && mousePosition[1] >= 0.0f) ? 1 : 0);
        m_Shader->SetUniform2f("u_MousePos", mousePosition[0], mousePosition[1]);
        m_Shader->SetUniform2f("u_ViewportResolution", viewport->GetWidth(), viewport->GetHeight());
        m_Shader->SetUniform1i("u_RequiresDrawBrush", m_DrawBrushSettings && m_DrawBrushSettings->m_ShowBrushCursor && viewport->IsHovered());
        m_Shader->SetUniform1i("u_DrawMask", m_DrawBrushSettings && m_DrawBrushSettings->m_ShowMask && m_DrawBrushSettings->m_MaskTexture != -1);
        if (m_DrawBrushSettings) {
            m_Shader->SetUniform4f("u_BrushSettings0", m_DrawBrushSettings->m_BrushPositionX,
                                   m_DrawBrushSettings->m_BrushPositionY,
                                   m_DrawBrushSettings->m_BrushSize,
                                   m_DrawBrushSettings->m_BrushFalloff);
            m_Shader->SetUniform3f("u_MaskColor", m_DrawBrushSettings->m_MaskColor);
            m_Shader->SetUniform1i("u_InvertMask", m_DrawBrushSettings->m_InvertMask ? 1 : 0);

            if (m_DrawBrushSettings->m_MaskTexture != -1) {
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, m_DrawBrushSettings->m_MaskTexture);
                m_Shader->SetUniform1i("u_MaskTexture", 4);
            }
        }

        auto *rendererLights = m_AppState->rendererManager->GetRendererLights();
        m_Shader->SetUniform3f("u_SunDirection", rendererLights->GetSunDirection());
        m_Shader->SetUniform3f("u_SunColor", rendererLights->GetSunColor());
        m_Shader->SetUniform1f("u_SunIntensity", rendererLights->GetSunIntensity());
        m_Shader->SetUniform1i("u_EnableSkyLight",
                               (rendererLights->IsSkyLightEnabled() &&
                                 m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady())
                                    ? 1
                                    : 0);
        m_Shader->SetUniform1f("u_SkyLightIntensity",
                               rendererLights->GetSkyLightIntensity());

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady() ? m_AppState->rendererManager->GetSkyRenderer()->GetIrradianceMap() : 0);
        m_Shader->SetUniform1i("u_IrradianceMap", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady() ? m_AppState->rendererManager->GetSkyRenderer()->GetSpecularMap() : 0);
        m_Shader->SetUniform1i("u_SpecularMap", 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady() ? m_AppState->rendererManager->GetSkyRenderer()->GetBrdfLut() : 0);
        m_Shader->SetUniform1i("u_BrdfLut", 3);
        TF3D_PROFILE_END(objectMaterialStateProfile);

        {
            TF3D_PROFILE_SCOPE("renderer/object/terrain-draw");
            m_AppState->mainModel->Render();
            m_SharedMemoryBuffer->GetData(viewport->GetPositionOnTerrain().data(), sizeof(float) * 4);
        }

        // m_CustomBaseShapeDrawSettings = nullptr;
    }

    void ObjectRenderer::ShowSettings()
    {
        ImGui::Checkbox("Invert Normals", &m_InvertNormals);
        if (ImGui::Checkbox("View Normals", &m_ViewNormals) && m_ViewNormals)
            m_ViewSlope = false;
        if (ImGui::Checkbox("View Slope", &m_ViewSlope) && m_ViewSlope)
            m_ViewNormals = false;
        ImGui::Checkbox("View Terrain Self Shadow", &m_ViewTerrainSelfShadow);
        ImGui::Checkbox("View Terrain Ambient", &m_ViewTerrainAmbient);
        ImGui::Checkbox("View Terrain Bent Normal", &m_ViewTerrainBentNormal);
        if (ImGui::Button("Reload Shaders"))
            ReloadShaders();
    }

    void ObjectRenderer::ReloadShaders()
    {
        m_Shader            = m_AppState->resourceManager->LoadShader("object_mode", true);
        m_PostProcessShader = m_AppState->resourceManager->LoadShader("post_process/base_surface", true);
    }
} // namespace tf3d::renderer
