#include "Renderer/RendererViewport.h"

RendererViewport::RendererViewport()
{
	m_Width = m_Height = 1024;
	m_FrameBuffer = std::make_shared<FrameBuffer>(1024, 1024);
}

RendererViewport::~RendererViewport()
{
}

void RendererViewport::ResizeTo(uint32_t width, uint32_t height)
{
	m_Width = width; m_Height = height;
	m_FrameBuffer = std::make_shared<FrameBuffer>(width, height);
}
