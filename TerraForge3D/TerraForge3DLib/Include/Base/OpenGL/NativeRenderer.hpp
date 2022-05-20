#pragma once
#include "Base/OpenGL/Core.hpp"
#include "Base/Renderer/NativeRenderer.hpp"

#ifdef TF3D_OPENGL_BACKEND

namespace TerraForge3D
{

	namespace OpenGL
	{
		class NativeRenderer : public RendererAPI::NativeRenderer
		{
		public:
			NativeRenderer();
			~NativeRenderer();

			virtual void Flush() override;

		private:
		};

	}

}

#endif