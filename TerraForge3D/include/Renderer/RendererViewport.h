#pragma once

#include "Base/Base.h"

/* Different modes based on shaders/outputs */
enum RendererViewportMode
{
	RendererViewportMode_Object = 0,
	RendererViewportMode_Wireframe,
	RendererViewportMode_Heightmap,
	RendererViewportMode_Shaded,
	RendererViewportMode_TextureSlot,
	RendererViewportMode_COUNT
};

/* A viewport object for rendering a instance of scene with its own camera/context */
class RendererViewport
{
public:
	RendererViewport();
	~RendererViewport();	
	void ResizeTo(uint32_t width, uint32_t height);

public:
	FrameBuffer* m_FrameBuffer = nullptr; // Framebuffer for this viewport
	Camera m_Camera; // Camera for this viewport
	RendererViewportMode m_ViewportMode = RendererViewportMode_Object; // Viewport mode
	uint32_t m_TextureSlot = 1;
	float m_OffsetX = 0.0f, m_OffsetY = 0.0f, m_Scale = 1.0;
};