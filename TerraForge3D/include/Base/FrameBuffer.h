#pragma once

#include <cstdint>

class FrameBuffer
{

public:

	FrameBuffer(int width = 800, int height = 600);
	~FrameBuffer();

	void Begin();
	void Resolve();
	uint32_t End();

	uint32_t GetColorTexture();
	uint32_t GetDepthTexture();
	uint32_t GetRendererID();

	inline int GetWidth()
	{
		return width;
	}
	inline int GetHeight()
	{
		return height;
	}

private:
	uint32_t colorTexture, multisampleColorTexture, depthTexture, fbo, resolveFbo;
	int width, height;
};
