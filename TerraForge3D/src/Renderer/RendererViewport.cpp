#include "Renderer/RendererViewport.h"

namespace tf3d::renderer
{
    RendererViewport::RendererViewport()
    {
        m_Shared.width       = 1024;
        m_Shared.height      = 1024;
        m_Shared.frameBuffer = std::make_shared<FrameBuffer>(1024, 1024);
    }

    RendererViewport::~RendererViewport()
    {
    }

    void RendererViewport::ResizeTo(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return;
        if (m_Shared.frameBuffer && m_Shared.width == static_cast<int32_t>(width) &&
            m_Shared.height == static_cast<int32_t>(height))
            return;

        m_Shared.width          = static_cast<int32_t>(width);
        m_Shared.height         = static_cast<int32_t>(height);
        m_ImageMode.aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        m_Shared.frameBuffer    = std::make_shared<FrameBuffer>(width, height);
    }

    std::shared_ptr<FrameBuffer> &RendererViewport::GetFrameBuffer()
    {
        return m_Shared.frameBuffer;
    }

    const std::shared_ptr<FrameBuffer> &RendererViewport::GetFrameBuffer() const
    {
        return m_Shared.frameBuffer;
    }

    Camera &RendererViewport::GetCamera()
    {
        return m_SceneMode.camera;
    }

    const Camera &RendererViewport::GetCamera() const
    {
        return m_SceneMode.camera;
    }

    RendererViewportMode RendererViewport::GetMode() const
    {
        return m_Mode;
    }

    void RendererViewport::SetMode(RendererViewportMode mode)
    {
        m_Mode = mode;
    }

    std::array<float, 4> &RendererViewport::GetPositionOnTerrain()
    {
        return m_SceneMode.positionOnTerrain;
    }

    const std::array<float, 4> &RendererViewport::GetPositionOnTerrain() const
    {
        return m_SceneMode.positionOnTerrain;
    }

    float &RendererViewport::GetOffsetX()
    {
        return m_ImageMode.offsetX;
    }

    float &RendererViewport::GetOffsetY()
    {
        return m_ImageMode.offsetY;
    }

    float &RendererViewport::GetScale()
    {
        return m_ImageMode.scale;
    }

    float RendererViewport::GetAspectRatio() const
    {
        return m_ImageMode.aspectRatio;
    }

    void RendererViewport::SetAspectRatio(float aspectRatio)
    {
        m_ImageMode.aspectRatio = aspectRatio;
    }

    std::array<float, 2> &RendererViewport::GetMousePosition()
    {
        return m_Shared.mousePosition;
    }

    const std::array<float, 2> &RendererViewport::GetMousePosition() const
    {
        return m_Shared.mousePosition;
    }

    int32_t RendererViewport::GetWidth() const
    {
        return m_Shared.width;
    }

    int32_t RendererViewport::GetHeight() const
    {
        return m_Shared.height;
    }

    bool RendererViewport::IsHovered() const
    {
        return m_Shared.isHovered;
    }

    void RendererViewport::SetHovered(bool hovered)
    {
        m_Shared.isHovered = hovered;
    }

    bool &RendererViewport::GetTextureSlotDetailedMode()
    {
        return m_TextureSlotMode.detailedMode;
    }

    int32_t &RendererViewport::GetTextureSlot()
    {
        return m_TextureSlotMode.textureSlot;
    }

    std::array<std::pair<int32_t, int32_t>, 4> &RendererViewport::GetTextureSlotDetailed()
    {
        return m_TextureSlotMode.detailed;
    }

    const std::array<std::pair<int32_t, int32_t>, 4> &RendererViewport::GetTextureSlotDetailed() const
    {
        return m_TextureSlotMode.detailed;
    }

} // namespace tf3d::renderer
