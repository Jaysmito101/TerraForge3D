#pragma once
#include "Base/OpenGL/Core.hpp"
#include "Base/Renderer/NativeMesh.hpp"

#ifdef TF3D_OPENGL_BACKEND
namespace TerraForge3D
{

	namespace OpenGL
	{

		class NativeMesh : public RendererAPI::NativeMesh
		{
		public:
			NativeMesh() = default;
			~NativeMesh();

			virtual bool Setup() override;
			virtual bool Destroy() override;
			virtual bool UploadData(void* vertices, void* indices) override;

		public:
			GLuint vertexArray = 0;
			GLuint vertexBuffer = 0;
			GLuint indexBuffer = 0;			
			size_t triangleCount = 0;
		};

	}

}
#endif