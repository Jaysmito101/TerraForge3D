#include "Renderer/HeightmapRenderer.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

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
    glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Resolution"), m_AppState->mainMap.tileResolution);
    glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileSize"), m_AppState->mainMap.tileSize);
    glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileOffset"), m_AppState->mainMap.tileOffsetX, m_AppState->mainMap.tileOffsetY);
    glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_HeightmapMin"), fieldMinimum);
    glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_HeightmapMax"), fieldMaximum);
    glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_AspectRatio"), ((float)viewport->m_AspectRatio));
    glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Offset"), viewport->m_OffsetX, viewport->m_OffsetY);
    glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Scale"), viewport->m_Scale);
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
    // m_Shader = new Shader(
    //	ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "heightmap_mode" PATH_SEPARATOR "vert.glsl", &success),
    //	ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "heightmap_mode" PATH_SEPARATOR "frag.glsl", &success)
    //);

    m_Shader = m_AppState->resourceManager->LoadShader("heightmap_mode", true);
}
