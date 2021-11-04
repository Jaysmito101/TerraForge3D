#pragma once

#include <cstdint>

class FrameBuffer {

public:

	FrameBuffer(int width = 800, int height = 600);
	~FrameBuffer();

	void Begin();
	uint32_t End();

	uint32_t GetColorTexture();
	uint32_t GetDepthTexture();
	uint32_t GetRendererID();

private:
	uint32_t colorTexture, depthTexture, fbo;
	int width, height;
};
