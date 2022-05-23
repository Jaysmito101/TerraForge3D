#pragma once
#include "Base/Renderer/Framebuffer.hpp"
#include "Base/OpenGL/Core.hpp"

#ifdef TF3D_OPENGL_BACKEND

namespace TerraForge3D
{

	namespace OpenGL
	{

		class FrameBuffer : public RendererAPI::FrameBuffer
		{
		public:
			FrameBuffer();
			~FrameBuffer();

			virtual void Setup() override;
			virtual void Destroy() override;
			virtual void Clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) override;
			virtual void* GetNativeHandle(void* handle = nullptr) override;
			virtual void* GetColorAttachmentHandle(int index, void* handle = nullptr) override;
			virtual void* GetDepthAttachmentHandle(void* handle = nullptr) override;
			virtual void* ReadPixel(uint32_t x, uint32_t y, int index = 0, void* data = nullptr) override;

		// private:
		public:
			GLuint colorAttachments[4] = {0, 0, 0, 0};
			GLuint depthAttachment = 0;
			GLuint handle = 0;
		};

	}

}

#endif