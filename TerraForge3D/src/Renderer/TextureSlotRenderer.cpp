#include "Renderer/TextureSlotRenderer.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

namespace tf3d::renderer
{

    TextureSlotRenderer::TextureSlotRenderer(ApplicationState *appState)
    {
        m_AppState   = appState;
        m_ScreenQuad = std::make_shared<Model>("Heightmap-Renderer-Screen-Quad");
        m_ScreenQuad->mesh->GenerateScreenQuad();
        m_ScreenQuad->SetupMeshOnGPU();
        m_ScreenQuad->UploadToGPU();
        ReloadShaders();
    }

    TextureSlotRenderer::~TextureSlotRenderer()
    {
    }

    void TextureSlotRenderer::Render(RendererViewport *viewport)
    {
        m_Shader->Bind();
        m_AppState->generationManager->GetHeightmapData()->Bind(0);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Resolution"), m_AppState->mainMap.tileResolution);
        glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileSize"), m_AppState->mainMap.tileSize);
        glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TileOffset"), m_AppState->mainMap.tileOffsetX, m_AppState->mainMap.tileOffsetY);
        glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_AspectRatio"), viewport->GetAspectRatio());
        glUniform2f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Offset"), viewport->GetTextureSlotOffsetX(), viewport->GetTextureSlotOffsetY());
        glUniform1f(glGetUniformLocation(m_Shader->GetNativeShader(), "u_Scale"), viewport->GetTextureSlotScale());
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TextureSlotDetailedMode"), viewport->GetTextureSlotDetailedMode() ? GL_TRUE : GL_FALSE);
        glUniform1i(glGetUniformLocation(m_Shader->GetNativeShader(), "u_TextureSlot"), viewport->GetTextureSlot());
        const auto &textureSlotDetailed = viewport->GetTextureSlotDetailed();
        for (int i = 0; i < 4; i++)
            glUniform2i(glGetUniformLocation(m_Shader->GetNativeShader(), ("u_TextureSlotDetailed[" + std::to_string(i) + "]").c_str()), textureSlotDetailed[i].first, textureSlotDetailed[i].second);
        m_ScreenQuad->Render();
    }

    void TextureSlotRenderer::ShowSettings()
    {
        if (ImGui::Button("Reload Shaders"))
            ReloadShaders();
    }

    void TextureSlotRenderer::ReloadShaders()
    {
        m_Shader = m_AppState->resourceManager->LoadShader("texture_slot_mode", true);
    }

} // namespace tf3d::renderer
