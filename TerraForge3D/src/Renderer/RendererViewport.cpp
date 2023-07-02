#include "Renderer/RendererViewport.h"

RendererViewport::RendererViewport()
{
	m_FrameBuffer = std::make_shared<FrameBuffer>(512, 512);
	m_Width = m_Height = 512;
}

RendererViewport::~RendererViewport()
{
}

void RendererViewport::ResizeTo(uint32_t width, uint32_t height)
{
	m_Width = width; 
	m_Height = height;
	m_FrameBuffer = std::make_shared<FrameBuffer>(width, height);
}
