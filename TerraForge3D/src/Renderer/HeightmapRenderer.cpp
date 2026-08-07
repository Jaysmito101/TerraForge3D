#include "Renderer/HeightmapRenderer.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

namespace tf3d::renderer
{
    HeightmapRenderer::HeightmapRenderer(ApplicationState *appState)
    {
        m_AppState   = appState;
        m_ScreenQuad = std::make_shared<Model>("Heightmap-Renderer-Screen-Quad");
        m_ScreenQuad->mesh->GenerateScreenQuad();
        m_ScreenQuad->SetupMeshOnGPU();
        m_ScreenQuad->UploadToGPU();
        ReloadShaders();
    }

    HeightmapRenderer::~HeightmapRenderer()
    {
    }

    void HeightmapRenderer::Render(RendererViewport *viewport)
    {
        m_Shader->Bind();
        m_AppState->generationManager->GetHeightmapData()->Bind(0);
        const auto &fieldStatistics = m_AppState->generationManager->GetFieldStatisticsResult();
        const float fieldMinimum    = fieldStatistics.valid ? fieldStatistics.minimum : 0.0f;
        const float fieldMaximum    = fieldStatistics.valid ? fieldStatistics.maximum : 1.0f;
        m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
        m_Shader->SetUniform1f("u_TileSize", m_AppState->mainMap.tileSize);
        m_Shader->SetUniform2f("u_TileOffset", m_AppState->mainMap.tileOffsetX,
                               m_AppState->mainMap.tileOffsetY);
        m_Shader->SetUniform1f("u_HeightmapMin", fieldMinimum);
        m_Shader->SetUniform1f("u_HeightmapMax", fieldMaximum);
        m_Shader->SetUniform1f("u_AspectRatio", viewport->GetAspectRatio());
        m_Shader->SetUniform2f("u_Offset", viewport->GetHeightmapOffsetX(),
                               viewport->GetHeightmapOffsetY());
        m_Shader->SetUniform1f("u_Scale", viewport->GetHeightmapScale());
        m_ScreenQuad->Render();
    }

    void HeightmapRenderer::ShowSettings()
    {
        const auto &fieldStatistics = m_AppState->generationManager->GetFieldStatisticsResult();
        if (fieldStatistics.valid)
            ImGui::Text("Using final field range: %.4f to %.4f", fieldStatistics.minimum, fieldStatistics.maximum);
        else
            ImGui::TextUnformatted("Final field range unavailable");
        if (ImGui::Button("Reload Shaders"))
            ReloadShaders();
    }

    void HeightmapRenderer::ReloadShaders()
    {
        // if (m_Shader) delete m_Shader;
        // bool success = false;
        // m_Shader = new GraphicsShader(
        //	ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "heightmap_mode" PATH_SEPARATOR "vert.glsl", &success),
        //	ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "heightmap_mode" PATH_SEPARATOR "frag.glsl", &success)
        //);

        m_Shader = m_AppState->resourceManager->LoadGraphicsShader("heightmap_mode", true);
    }
} // namespace tf3d::renderer
