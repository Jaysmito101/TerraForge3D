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

		private:
		};

	}

}

#endif