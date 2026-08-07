#pragma once

#include "Base/Base.h"
#include "Renderer/RendererViewport.h"

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::renderer
{
    class RendererBase
    {
    public:
        RendererBase()          = default;
        virtual ~RendererBase() = default;

        virtual void Render(RendererViewport *viewport) = 0;
        virtual void ShowSettings()                     = 0;

    protected:
        virtual void ReloadShaders() = 0;

    protected:
        ApplicationState *m_AppState                   = nullptr;
        std::shared_ptr<base::GraphicsShader> m_Shader = nullptr;
    };
} // namespace tf3d::renderer
