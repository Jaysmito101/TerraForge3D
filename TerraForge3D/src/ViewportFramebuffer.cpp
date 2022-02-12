#include "ViewportFramebuffer.h"
#include <glad/glad.h>
#include <FrameBuffer.h>

FrameBuffer* fb;


void SetupViewportFrameBuffer()
{
	fb = new FrameBuffer();
}

uint32_t GetViewportFramebufferId()
{
	return fb->GetRendererID();
}

uint32_t GetViewportFramebufferColorTextureId()
{
	return fb->GetColorTexture();
}

uint32_t GetViewportFramebufferDepthTextureId()
{
	return fb->GetDepthTexture();
}
