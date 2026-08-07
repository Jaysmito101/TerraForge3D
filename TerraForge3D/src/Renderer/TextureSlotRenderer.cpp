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
        m_Shader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
        m_Shader->SetUniform1f("u_TileSize", m_AppState->mainMap.tileSize);
        m_Shader->SetUniform2f("u_TileOffset", m_AppState->mainMap.tileOffsetX,
                               m_AppState->mainMap.tileOffsetY);
        m_Shader->SetUniform1f("u_AspectRatio", viewport->GetAspectRatio());
        m_Shader->SetUniform2f("u_Offset", viewport->GetTextureSlotOffsetX(),
                               viewport->GetTextureSlotOffsetY());
        m_Shader->SetUniform1f("u_Scale", viewport->GetTextureSlotScale());
        m_Shader->SetUniform1i("u_TextureSlotDetailedMode",
                               viewport->GetTextureSlotDetailedMode() ? 1 : 0);
        m_Shader->SetUniform1i("u_TextureSlot", viewport->GetTextureSlot());
        const auto &textureSlotDetailed = viewport->GetTextureSlotDetailed();
        for (int i = 0; i < 4; i++)
            m_Shader->SetUniform2i("u_TextureSlotDetailed[" + std::to_string(i) + "]",
                                   textureSlotDetailed[i].first, textureSlotDetailed[i].second);
        m_ScreenQuad->Render();
    }

    void TextureSlotRenderer::ShowSettings()
    {
        if (ImGui::Button("Reload Shaders"))
            ReloadShaders();
    }

    void TextureSlotRenderer::ReloadShaders()
    {
        m_Shader = m_AppState->resourceManager->LoadGraphicsShader("texture_slot_mode", true);
    }

} // namespace tf3d::renderer
