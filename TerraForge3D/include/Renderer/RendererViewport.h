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
	std::shared_ptr<FrameBuffer> m_FrameBuffer; // Framebuffer for this viewport
	Camera m_Camera; // Camera for this viewport
	RendererViewportMode m_ViewportMode = RendererViewportMode_Object; // Viewport mode
	float m_PosOnTerrain[4]  = { 0.0f, 0.0f, 0.0f }; // Position on terrain
	bool m_TextureSlotDetailedMode = false; // Detailed mode for texture slot renderer
	int32_t m_TextureSlot = 0; // Texture slot to render
	std::pair<int32_t, int32_t> m_TextureSlotDetailed[4];
	float m_OffsetX = 0.0f, m_OffsetY = 0.0f, m_Scale = 1.0;
	float m_AspectRatio = 1.0f;
	float m_MousePosition[2] = { 0.0f, 0.0f };
	int32_t m_Width = 0, m_Height = 0;
};