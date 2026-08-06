#pragma once

#include "Renderer/RendererBase.h"

namespace tf3d::renderer
{

    class HeightmapRenderer : public RendererBase
    {
    public:
        HeightmapRenderer(ApplicationState *appState);
        virtual ~HeightmapRenderer();

        virtual void Render(RendererViewport *viewport) override;
        virtual void ShowSettings() override;

    private:
        virtual void ReloadShaders() override;

    private:
        std::shared_ptr<Model> m_ScreenQuad = nullptr;
    };
} // namespace tf3d::renderer