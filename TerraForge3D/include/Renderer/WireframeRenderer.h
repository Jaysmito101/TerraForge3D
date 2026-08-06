#pragma once

#include "Renderer/RendererBase.h"

namespace tf3d::renderer
{
    class WireframeRenderer : public RendererBase
    {
    public:
        WireframeRenderer(ApplicationState *appState);
        virtual ~WireframeRenderer();

        virtual void Render(RendererViewport *viewport) override;
        virtual void ShowSettings() override;

    protected:
        virtual void ReloadShaders() override;

    private:
        bool m_InvertNormals = false;
    };
} // namespace tf3d::renderer