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
	bool m_TextureSlotDetailedMode = false; // Detailed mode for texture slot renderer
	int32_t m_TextureSlot = 0; // Texture slot to render
	std::pair<int32_t, int32_t> m_TextureSlotDetailed[4];
	float m_OffsetX = 0.0f, m_OffsetY = 0.0f, m_Scale = 1.0;
	float m_AspectRatio = 1.0f;
};