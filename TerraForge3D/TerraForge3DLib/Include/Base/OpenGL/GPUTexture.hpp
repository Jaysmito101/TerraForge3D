#pragma once
#include "Base/OpenGL/Core.hpp"
#include "Base/Renderer/GPUTexture.hpp"

#ifdef TF3D_OPENGL_BACKEND

namespace TerraForge3D
{

	namespace OpenGL
	{

		class GPUTexture : public RendererAPI::GPUTexture
		{
		public:
			GPUTexture();
			~GPUTexture();

			virtual void Setup() override;
			virtual void Destroy() override;
			virtual void SetData(void* data) override;
			virtual void SetData(void* data, uint32_t depth) override;
			virtual void SetData(void* data, RendererAPI::GPUTextureCubemapFace face) override;
			virtual void GetData(void* data) override;
			virtual void GetHandle(void* handle) override;
			virtual ImTextureID GetImGuiID() override;


			void UpdateInfo();

		public:
			GLuint handle = 0;
			GLuint target = GL_TEXTURE_2D;
			GLuint internalFormat = GL_RGB8;
			GLuint textureFormat = GL_RGB;
			GLuint dataType = GL_UNSIGNED_BYTE;

		};

	}

}


#endif