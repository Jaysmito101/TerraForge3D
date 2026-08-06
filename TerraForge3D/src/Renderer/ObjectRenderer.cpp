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
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TerrainSelfShadow"), 6);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_HasTerrainSelfShadow"), hasTerrainSelfShadow ? 1 : 0);
        auto *heightfieldAmbient         = m_AppState->rendererManager->GetHeightfieldAmbientCache();
        const bool hasHeightfieldAmbient = heightfieldAmbient != nullptr && heightfieldAmbient->IsReady();
        if (hasHeightfieldAmbient) {
            heightfieldAmbient->Bind(8);
        } else {
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TerrainAmbient"), 8);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_HasTerrainAmbient"), hasHeightfieldAmbient ? 1 : 0);
        auto *heightfieldGI         = m_AppState->rendererManager->GetHeightfieldGICache();
        const bool hasHeightfieldGI = heightfieldGI != nullptr && heightfieldGI->IsReady();
        if (hasHeightfieldGI) {
            heightfieldGI->Bind(9);
        } else {
            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TerrainGI"), 9);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_HasTerrainGI"), hasHeightfieldGI ? 1 : 0);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SlopeTexture"), 5);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_HasSlopeTexture"), hasSlopeTexture ? 1 : 0);
        // glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Projection"), 1, GL_FALSE, glm::value_ptr(viewport->GetCamera().pers));
        // glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_View"), 1, GL_FALSE, glm::value_ptr(viewport->GetCamera().view));
        glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ProjectionView"), 1, GL_FALSE, glm::value_ptr(viewport->GetCamera().GetProjectionViewMatrix()));
        glUniform3fv(glGetUniformLocation(m_Shader->GetNativeShader(), "u_CameraPosition"), 1, glm::value_ptr(viewport->GetCamera().GetPosition()));
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Resolution"), m_AppState->mainMap.tileResolution);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_InvertNormals"), m_InvertNormals);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ViewNormals"), m_ViewNormals ? 1 : 0);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ViewSlope"), m_ViewSlope ? 1 : 0);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ViewTerrainSelfShadow"), m_ViewTerrainSelfShadow ? 1 : 0);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ViewTerrainAmbient"), m_ViewTerrainAmbient ? 1 : 0);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ViewTerrainBentNormal"), m_ViewTerrainBentNormal ? 1 : 0);
        glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileSize"), m_AppState->mainMap.tileSize);
        glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileOffset"), m_AppState->mainMap.tileOffsetX, m_AppState->mainMap.tileOffsetY);
        const bool isPlane          = m_AppState->mainModel != nullptr && m_AppState->mainModel->isGeneratedPlane;
        const auto &fieldStatistics = m_AppState->generationManager->GetFieldStatisticsResult();
        const float fieldMinimum    = fieldStatistics.valid ? fieldStatistics.minimum : 0.0f;
        const float solidDepth      = isPlane ? std::max(m_AppState->mainModel->planeSolidDepth, 0.0001f) : 0.0f;
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_PlaneMode"), isPlane ? 1 : 0);
        glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_FieldMinimum"), fieldMinimum);
        glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_HeightOffset"), isPlane ? -fieldMinimum + solidDepth : 0.0f);
        glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SolidDepth"), solidDepth);
        TF3D_PROFILE_END(objectSetupProfile);
        {
            TF3D_PROFILE_SCOPE("renderer/object/post-process");
            if (isPlane && m_PostProcessShader != nullptr && viewport->GetCamera().GetPosition().y > 0.0f) {
                m_PostProcessShader->Bind();
                const glm::mat4 inverseProjectionView = glm::inverse(viewport->GetCamera().GetProjectionViewMatrix());
                glUniformMatrix4fv(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_InverseProjectionView"), 1, GL_FALSE, glm::value_ptr(inverseProjectionView));
                glUniformMatrix4fv(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_ProjectionView"), 1, GL_FALSE, glm::value_ptr(viewport->GetCamera().GetProjectionViewMatrix()));
                glUniform3fv(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_CameraPosition"), 1, glm::value_ptr(viewport->GetCamera().GetPosition()));
                glUniform2f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_ViewportResolution"), viewport->GetWidth(), viewport->GetHeight());
                glUniform3f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_Color"),
                            67.0f / 255.0f, 88.0f / 255.0f, 114.0f / 255.0f);
                auto *rendererLights      = m_AppState->rendererManager->GetRendererLights();
                auto *skyRenderer         = m_AppState->rendererManager->GetSkyRenderer();
                const bool enableSkyLight = rendererLights->m_UseSkyLight && skyRenderer->IsSkyReady();
                const auto &baseSun       = rendererLights->m_Sun;
                glUniform1i(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_EnableSkyLight"), enableSkyLight ? 1 : 0);
                glUniform1f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_SkyLightIntensity"), rendererLights->m_SkyLightIntensity);
                glUniform3f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_SunDirection"), baseSun.direction.x, baseSun.direction.y, baseSun.direction.z);
                glUniform3f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_SunColor"), baseSun.color.x, baseSun.color.y, baseSun.color.z);
                glUniform1f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_SunIntensity"), baseSun.intensity);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, enableSkyLight ? skyRenderer->GetIrradianceMap() : 0);
                glUniform1i(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_IrradianceMap"), 1);
                auto *planarShadowCache    = m_AppState->rendererManager->GetPlanarShadowCache();
                const bool hasPlanarShadow = planarShadowCache != nullptr && planarShadowCache->IsReady();
                if (hasPlanarShadow) {
                    planarShadowCache->Bind(7);
                } else {
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                glUniform1i(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_TerrainPlanarShadow"), 7);
                glUniform1i(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_HasTerrainPlanarShadow"), hasPlanarShadow ? 1 : 0);
                glUniform1f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_PlanarShadowSoftness"),
                            m_AppState->rendererManager->GetPlanarShadowSoftness());
                if (hasPlanarShadow) {
                    const auto atlasMinimum   = planarShadowCache->GetAtlasMinimumXZ();
                    const auto atlasWorldSize = planarShadowCache->GetAtlasWorldSize();
                    glUniform2f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_PlanarShadowMinimumXZ"), atlasMinimum.x, atlasMinimum.y);
                    glUniform2f(glGetUniformLocation(m_PostProcessShader->GetNativeShader(), "u_PlanarShadowWorldSize"), atlasWorldSize.x, atlasWorldSize.y);
                }
                glBindVertexArray(m_PostProcessVao);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glBindVertexArray(0);

                m_Shader->Bind();
            }
        }

        TF3D_PROFILE_BEGIN(objectMaterialStateProfile, "renderer/object/material-state");
        const auto &mousePosition = viewport->GetMousePosition();
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_IsViewportActive"), (mousePosition[0] >= 0.0f && mousePosition[1] >= 0.0f) ? 1 : 0);
        glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_MousePos"), mousePosition[0], mousePosition[1]);
        glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_ViewportResolution"), viewport->GetWidth(), viewport->GetHeight());
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_RequiresDrawBrush"), m_DrawBrushSettings && m_DrawBrushSettings->m_ShowBrushCursor && viewport->IsHovered());
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_DrawMask"), m_DrawBrushSettings && m_DrawBrushSettings->m_ShowMask && m_DrawBrushSettings->m_MaskTexture != -1);
        if (m_DrawBrushSettings) {
            glUniform4f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_BrushSettings0"), m_DrawBrushSettings->m_BrushPositionX, m_DrawBrushSettings->m_BrushPositionY, m_DrawBrushSettings->m_BrushSize, m_DrawBrushSettings->m_BrushFalloff);
            glUniform3f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_MaskColor"), m_DrawBrushSettings->m_MaskColor.x, m_DrawBrushSettings->m_MaskColor.y, m_DrawBrushSettings->m_MaskColor.z);
            glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_InvertMask"), m_DrawBrushSettings->m_InvertMask ? 1 : 0);

            if (m_DrawBrushSettings->m_MaskTexture != -1) {
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, m_DrawBrushSettings->m_MaskTexture);
                glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_MaskTexture"), 4);
            }
        }

        const auto &sun = m_AppState->rendererManager->GetRendererLights()->m_Sun;
        glUniform3f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SunDirection"), sun.direction.x, sun.direction.y, sun.direction.z);
        glUniform3f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SunColor"), sun.color.x, sun.color.y, sun.color.z);
        glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SunIntensity"), sun.intensity);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_EnableSkyLight"), (m_AppState->rendererManager->GetRendererLights()->m_UseSkyLight && m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady()) ? GL_TRUE : GL_FALSE);
        glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SkyLightIntensity"), m_AppState->rendererManager->GetRendererLights()->m_SkyLightIntensity);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady() ? m_AppState->rendererManager->GetSkyRenderer()->GetIrradianceMap() : 0);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_IrradianceMap"), 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady() ? m_AppState->rendererManager->GetSkyRenderer()->GetSpecularMap() : 0);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_SpecularMap"), 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, m_AppState->rendererManager->GetSkyRenderer()->IsSkyReady() ? m_AppState->rendererManager->GetSkyRenderer()->GetBrdfLut() : 0);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_BrdfLut"), 3);
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
