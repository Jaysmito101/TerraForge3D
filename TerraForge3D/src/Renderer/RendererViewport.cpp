#include "Renderer/RendererViewport.h"

namespace tf3d::renderer
{
    RendererViewport::RendererViewport()
    {
        m_Width = m_Height = 1024;
        m_FrameBuffer      = std::make_shared<FrameBuffer>(1024, 1024);
    }

    RendererViewport::~RendererViewport()
    {
    }

    void RendererViewport::ResizeTo(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return;
        if (m_FrameBuffer && m_Width == static_cast<int32_t>(width) && m_Height == static_cast<int32_t>(height))
            return;

        m_Width       = width;
        m_Height      = height;
        m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
        m_FrameBuffer = std::make_shared<FrameBuffer>(width, height);
    }

} // namespace tf3d::renderer