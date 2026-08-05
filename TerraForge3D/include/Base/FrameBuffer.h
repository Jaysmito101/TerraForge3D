#pragma once

#include <cstdint>

class FrameBuffer
{

public:

	FrameBuffer(int width = 800, int height = 600);
	~FrameBuffer();

	void Begin();
	void ResolveColor();
	void ResolveDepth();
	void Resolve();
	uint32_t End();

	uint32_t GetColorTexture();
	uint32_t GetDepthTexture();
	uint32_t GetResolvedDepthTexture();
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
	uint32_t colorTexture = 0;
	uint32_t multisampleColorTexture = 0;
	uint32_t depthTexture = 0;
	uint32_t resolvedDepthTexture = 0;
	uint32_t fbo = 0;
	uint32_t resolveFbo = 0;
	int width, height;
};
