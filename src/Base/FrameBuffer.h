#pragma once

#include <cstdint>

class FrameBuffer {

public:

	FrameBuffer();
	~FrameBuffer();

	void Begin();
	uint32_t End();

	uint32_t GetColorTexture();
	uint32_t GetDepthTexture();
	uint32_t GetRendererID();

private:
	uint32_t colorTexture, depthTexture, fbo;
};
