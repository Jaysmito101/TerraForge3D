#pragma once
#include "Base/Core/Core.hpp"
#include <imgui/imgui.h>

namespace TerraForge3D
{

	namespace RendererAPI
	{

		enum FrameBufferAttachmentFormat
		{
			FrameBufferAttachmentFormat_RGBA8 = 0,
			FrameBufferAttachmentFormat_RGBA16,
			FrameBufferAttachmentFormat_RGBA32,
			FrameBufferAttachmentFormat_R32I
		};

		class FrameBuffer
		{
		public:
			FrameBuffer();
			virtual ~FrameBuffer();


			virtual void Setup() = 0; 
			virtual void Destroy() = 0;
			virtual void Clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) = 0;
			virtual void* GetNativeHandle(void* handle = nullptr) = 0;
			virtual ImTextureID GetColorAttachmentImGuiID(int index = 0) = 0;
			virtual void* GetColorAttachmentHandle(int index, void* handle = nullptr) = 0;
			virtual void* GetDepthAttachmentHandle(void* handle = nullptr) = 0;
			virtual void* ReadPixel(uint32_t x, uint32_t y, int index = 0, void* data = nullptr) = 0;

			inline FrameBuffer* SetAutoDestroy(bool value) { TF3D_ASSERT(setupOnGPU == false, "Cannot change paramaeter after setup on GPU (call Destory first)"); this->autoDestroy = value; return this; }
			inline FrameBuffer* SetDepthAttachment(bool value) { TF3D_ASSERT(setupOnGPU == false, "Cannot change paramaeter after setup on GPU (call Destory first)"); this->hasDepthAttachment = value; return this; }
			inline FrameBuffer* SetColorAttachmentCount(int count = 1) { TF3D_ASSERT(setupOnGPU == false, "Cannot change paramaeter after setup on GPU (call Destory first)"); TF3D_ASSERT(count <= 4, "Maximum of only 4 color attachments are allowed"); this->colorAttachmentCount = count; return this; }
			inline FrameBuffer* SetColorAttachmentFormat(FrameBufferAttachmentFormat format, int index = 0) { TF3D_ASSERT(setupOnGPU == false, "Cannot change paramaeter after setup on GPU (call Destory first)"); TF3D_ASSERT(index < colorAttachmentCount, "Index out of range"); this->colorAttachmentFromats[index] = format; return this; }
			inline FrameBuffer* SetWidth(uint32_t value) { TF3D_ASSERT(setupOnGPU == false, "Cannot change paramaeter after setup on GPU (call Destory first)"); TF3D_ASSERT(value > 0, "Width cannot be negetive"); this->width = value; return this; }
			inline FrameBuffer* SetHeight(uint32_t value) { TF3D_ASSERT(setupOnGPU == false, "Cannot change paramaeter after setup on GPU (call Destory first)"); TF3D_ASSERT(value > 0, "Height cannot be negetive"); this->height = value; return this; }
			inline FrameBuffer* SetSize(uint32_t w, uint32_t h) { TF3D_ASSERT(setupOnGPU == false, "Cannot change paramaeter after setup on GPU (call Destory first)"); TF3D_ASSERT(w > 0 && h > 0, "Size cannot be negetive"); this->width = w; this->height = h; return this; }

			inline bool IsSetup() { return this->setupOnGPU; }
			inline bool GetAutoDestroy() { return this->autoDestroy; }
			inline bool GetDepthAttachment() { return this->hasDepthAttachment; }
			inline int  GetColorAttachmentCount() { return this->colorAttachmentCount; }
			inline std::pair<uint32_t, uint32_t> GetSize() { return { this->width, this->height }; }
			inline uint32_t GetWidth() { return this->width; }
			inline uint32_t GetHeight() { return this->height; }

		public:
			static FrameBuffer* Create();

		protected:
			bool autoDestroy = true;
			bool setupOnGPU = false;
			bool hasDepthAttachment = false;
			int colorAttachmentCount = 1;
			uint32_t width = 1024;
			uint32_t height = 1024;
			FrameBufferAttachmentFormat colorAttachmentFromats[4] = {FrameBufferAttachmentFormat_RGBA8};
		};

	}

}