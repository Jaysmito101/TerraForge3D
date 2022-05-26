#include "Base/OpenGL/Framebuffer.hpp"

#ifdef TF3D_OPENGL_BACKEND

namespace TerraForge3D
{

	namespace OpenGL
	{

		std::pair<GLenum, GLenum> GetOpenGLFormat(RendererAPI::FrameBufferAttachmentFormat format)
		{
			GLenum ifm = 0;
			GLenum fm = 0;

			switch (format)
			{
			case RendererAPI::FrameBufferAttachmentFormat_RGBA8:
				ifm = GL_RGBA8;
				fm = GL_RGBA;
				break;
			case RendererAPI::FrameBufferAttachmentFormat_RGBA16:
				ifm = GL_RGBA16;
				fm = GL_RGBA;
				break;
			case RendererAPI::FrameBufferAttachmentFormat_RGBA32:
				ifm = GL_FLOAT;
				fm = GL_RGBA;
				break;
			case RendererAPI::FrameBufferAttachmentFormat_R32I:
				ifm = GL_R32I;
				fm = GL_RED_INTEGER;
				break;

			default:
				TF3D_ASSERT(false, "Unknown FrameBuffer attachment format");
			}

			return {ifm, fm};
		}

		GLenum GetOpenGLType(RendererAPI::FrameBufferAttachmentFormat format)
		{
			switch (format)
			{
			case RendererAPI::FrameBufferAttachmentFormat_RGBA8:
			case RendererAPI::FrameBufferAttachmentFormat_RGBA16:
			case RendererAPI::FrameBufferAttachmentFormat_R32I:
				return GL_INT;
			case RendererAPI::FrameBufferAttachmentFormat_RGBA32:
				return GL_FLOAT;
			default:
				TF3D_ASSERT(false, "Unknown FrameBuffer attachment format");
			}
			return GL_NONE;
		}

		FrameBuffer::FrameBuffer()
		{
		}

		FrameBuffer::~FrameBuffer()
		{
			if (autoDestroy && setupOnGPU)
			{
				this->Destroy();
			}
		}

		void FrameBuffer::Setup()
		{
			TF3D_ASSERT(setupOnGPU == false, "FrameBuffer has already been setup call Destory first");
			TF3D_ASSERT(colorAttachmentCount >= 1, "Color attachment count cannot be less than 1");

			// Create and bind a framebuffer object
			glGenFramebuffers(1, &handle);
			glBindFramebuffer(GL_FRAMEBUFFER, handle);

			// Setup color attachments

			// Create the color attachment textures together
			glGenTextures(colorAttachmentCount, colorAttachments);
			// Setup the color attachment textures one by one
			for (int i = 0; i < colorAttachmentCount; i++)
			{
				glBindTexture(GL_TEXTURE_2D, colorAttachments[i]);
				auto [internalFormat, format] =GetOpenGLFormat(colorAttachmentFromats[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorAttachments[i], 0);
			}

			// Setup depth  attachment
			
			//Create the dpeth attachment textures
			glGenTextures(1, &depthAttachment);
			
			glBindTexture(GL_TEXTURE_2D, depthAttachment);

			glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, width, height);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthAttachment, 0);

			if (colorAttachmentCount > 1)
			{
				GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
				glDrawBuffers(colorAttachmentCount, buffers);
			}

			TF3D_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

			// Unbind the newly created framebuffer object
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			setupOnGPU = true;
		}

		void FrameBuffer::Destroy()
		{
			TF3D_ASSERT(setupOnGPU, "FrameBuffer has not yet been setup call Setup first");
			glDeleteFramebuffers(1, &handle);
			glDeleteTextures(colorAttachmentCount, colorAttachments);
			if(hasDepthAttachment)
				glDeleteTextures(1, &depthAttachment);
			setupOnGPU = false;
		}

		void* FrameBuffer::ReadPixel(uint32_t x, uint32_t y, int index, void* data)
		{
			TF3D_ASSERT(index >= 0 && index < colorAttachmentCount, "Index out of range");
			glBindFramebuffer(GL_FRAMEBUFFER, handle);
			glReadBuffer(GL_COLOR_ATTACHMENT0 + index);
			uint32_t pixelData = 0;
			glReadPixels(x, y, 1, 1, GetOpenGLFormat(colorAttachmentFromats[index]).second, GetOpenGLType(colorAttachmentFromats[index]), &pixelData);
			if (data)
				memcpy(data, &pixelData, sizeof(pixelData));
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return (void*)(intptr_t)(pixelData);
		}

		void FrameBuffer::Clear(float r, float g, float b, float a)
		{
			float color[4] = {r, g, b, a};
			glBindFramebuffer(GL_FRAMEBUFFER, handle);
			glClearColor(r, g, b, a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			for (int i = 0; i < colorAttachmentCount; i++)
			{
				glClearBufferfv(GL_COLOR, i, color);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void* FrameBuffer::GetNativeHandle(void* h)
		{
			if (h)
				memcpy(h, &handle, sizeof(handle));
			return (void*)(intptr_t)(handle);
		}

		ImTextureID FrameBuffer::GetColorAttachmentImGuiID(int index)
		{
			TF3D_ASSERT(index >= 0 && index < colorAttachmentCount, "Color attachment index out of range");
			return (ImTextureID)(intptr_t)(colorAttachments[index]);
		}
		
		void* FrameBuffer::GetColorAttachmentHandle(int index, void* h)
		{
			TF3D_ASSERT(index >= 0 && index < colorAttachmentCount, "Color attachment index out of range");
			if (h)
				memcpy(h, &colorAttachments[index], sizeof(colorAttachments[index]));
			return (void*)(intptr_t)(colorAttachments[index]);
		}

		void* FrameBuffer::GetDepthAttachmentHandle(void* h)
		{
			TF3D_ASSERT(hasDepthAttachment, "Depth attachment not enabled on this framebuffer");
			if (h)
				memcpy(h, &depthAttachment, sizeof(depthAttachment));
			return (void*)(intptr_t)(depthAttachment);
		}
	}

}

#endif