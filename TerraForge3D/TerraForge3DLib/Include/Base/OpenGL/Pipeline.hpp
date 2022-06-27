#pragma once
#include "Base/OpenGL/Core.hpp"
#include "Base/Renderer/Pipeline.hpp"

#ifdef TF3D_OPENGL_BACKEND

namespace TerraForge3D
{
	
	namespace RendererAPI
	{
		class FrameBuffer;
	}

	namespace OpenGL 
	{
		class FrameBuffer;

		class Pipeline : public RendererAPI::Pipeline
		{
		public:
			Pipeline();
			~Pipeline();

			virtual void Setup() override;
			virtual void Destory() override;
			virtual bool Rebuild(RendererAPI::FrameBuffer* framebuffer, bool forceRebuild = false) override;

		public:
			FrameBuffer* framebuffer = nullptr;
		};

	}

}

#endif