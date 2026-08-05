#include "FrameBuffer.h"

#include <glad/gl.h>

#include <algorithm>


FrameBuffer::FrameBuffer(int w, int h)
{
	width = w;
	height = h;
	GLint maxSamples = 1;
	glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
	const GLsizei samples = std::max<GLsizei>(1, std::min(8, static_cast<GLsizei>(maxSamples)));

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glGenTextures(1, &multisampleColorTexture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multisampleColorTexture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA16F, w, h, GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, multisampleColorTexture, 0);

	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depthTexture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_DEPTH_COMPONENT24, w, h, GL_TRUE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthTexture, 0);

	glGenFramebuffers(1, &resolveFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);
	glGenTextures(1, &colorTexture);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBuffer::~FrameBuffer()
{
	glDeleteTextures(1, &colorTexture);
	glDeleteTextures(1, &multisampleColorTexture);
	glDeleteTextures(1, &depthTexture);
	glDeleteFramebuffers(1, &fbo);
	glDeleteFramebuffers(1, &resolveFbo);
}

void FrameBuffer::Begin()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);
}

void FrameBuffer::ResolveColor()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void FrameBuffer::Resolve()
{
	ResolveColor();
}

uint32_t FrameBuffer::End()
{
	Resolve();
	return colorTexture;
}

uint32_t FrameBuffer::GetColorTexture()
{
	return colorTexture;
}

uint32_t FrameBuffer::GetDepthTexture()
{
	return depthTexture;
}

uint32_t FrameBuffer::GetRendererID()
{
	return fbo;
}
