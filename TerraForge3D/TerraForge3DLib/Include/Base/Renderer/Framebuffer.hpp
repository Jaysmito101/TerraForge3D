#pragma once
#include "Base/Core/Core.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{

		class FrameBuffer
		{
		public:
			FrameBuffer();
			~FrameBuffer();

			virtual void Setup() = 0; // TEMP

			// TODO: Implement

		public:
			static FrameBuffer* Create();
			static void Destroy(FrameBuffer* framebuffer);

		private:
		};

	}

}