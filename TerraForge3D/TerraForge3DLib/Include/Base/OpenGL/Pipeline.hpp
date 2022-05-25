#pragma once
#include "Base/OpenGL/Core.hpp"
#include "Base/Renderer/Pipeline.hpp"

#ifdef TF3D_OPENGL_BACKEND

namespace TerraForge3D
{

	namespace OpenGL 
	{

		class Pipeline : public RendererAPI::Pipeline
		{
		public:
			Pipeline();
			~Pipeline();

			virtual void Setup() override;
			virtual void Destory() override;

		public:

		};

	}

}

#endif