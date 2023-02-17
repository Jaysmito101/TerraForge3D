#include "Renderer/RendererViewport.h"

RendererViewport::RendererViewport()
{
	this->m_FrameBuffer = new FrameBuffer(512, 512);
}

RendererViewport::~RendererViewport()
{
	delete this->m_FrameBuffer;
}

void RendererViewport::ResizeTo(uint32_t width, uint32_t height)
{
	delete this->m_FrameBuffer;
	this->m_FrameBuffer = new FrameBuffer(width, height);
}
