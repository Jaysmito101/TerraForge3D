#pragma once

#include "Renderer/RendererBase.h"

class TextureSlotRenderer : public RendererBase
{
public:
    TextureSlotRenderer(ApplicationState *appState);
    virtual ~TextureSlotRenderer();

    virtual void Render(RendererViewport *viewport) override;
    virtual void ShowSettings() override;

private:
    virtual void ReloadShaders() override;

private:
    std::shared_ptr<Model> m_ScreenQuad = nullptr;
};
