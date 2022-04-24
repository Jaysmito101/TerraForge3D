#include "Base/OpenGL/GPUTexture.hpp"



namespace TerraForge3D
{

	namespace OpenGL
	{
		GPUTexture::GPUTexture()
		{
		}

		GPUTexture::~GPUTexture()
		{
			if (autoDestroy)
				Destroy();
		}

		void GPUTexture::Setup()
		{
			TF3D_ASSERT(!isSetupOnGPU, "Texture already setup on GPU (Call Destroy first)");
			TF3D_ASSERT(sizeX > 0 && sizeY > 0 && sizeZ > 0, "Texture sizes cannot be all 0");
			UpdateInfo();
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glGenTextures(1, &handle);
			glBindTexture(target, handle);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT);
			glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
			if (type == RendererAPI::GPUTextureType_3D || type == RendererAPI::GPUTextureType_Array)
				glTexImage3D(target, 0, internalFormat, sizeX, sizeY, sizeZ, 0, textureFormat, dataType, nullptr);
			else if (type == RendererAPI::GPUTextureType_2D )
				glTexImage2D(target, 0, internalFormat, sizeX, sizeY, 0, textureFormat, dataType, nullptr);
			
			glBindTexture(target, 0);
			isSetupOnGPU = true;
		}

		void GPUTexture::Destroy()
		{
			UpdateInfo();
			glDeleteTextures(1, &handle);
			isSetupOnGPU = false;
		}

		void GPUTexture::GetHandle(void* h)
		{
			memcpy(h, &handle, sizeof(handle));
		}

		void GPUTexture::SetData(void* data)
		{
			TF3D_ASSERT(type == RendererAPI::GPUTextureType_2D, "GPUTexture::SetData(void* data) is only for 2D textures");
			TF3D_ASSERT(isSetupOnGPU, "Forst call Setup");
			TF3D_ASSERT(sizeX > 0 && sizeY > 0 && sizeZ > 0, "Texture sizes cannot be all 0");
			TF3D_ASSERT(data, "Data is null");
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glBindTexture(target, handle);
			glTexImage2D(target, 0, internalFormat, sizeX, sizeY, 0, textureFormat, dataType, data);
			glBindTexture(target, 0);
		}

		void GPUTexture::SetData(void* data, uint32_t depth)
		{
			TF3D_ASSERT(type == RendererAPI::GPUTextureType_3D || type == RendererAPI::GPUTextureType_Array, "GPUTexture::SetData(void* data, uint32_t depth) is only for 3D textures and texture arrays");
			TF3D_ASSERT(isSetupOnGPU, "Texture not setup on GPU (First call Setup)");
			TF3D_ASSERT(sizeX > 0 && sizeY > 0 && sizeZ > 0, "Texture sizes cannot be all 0");
			TF3D_ASSERT(data, "Data is null");
			TF3D_ASSERT(depth >= 0, "Depth must be greater than or equal to 0");
			if (depth > 256)
			{
				TF3D_LOG_WARN("Using 3D texture with depth {0} (might be unsupported on certain gpus)", depth);
			}
			UpdateInfo();
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glBindTexture(target, handle);
			glTexSubImage3D(target, 0, 0, 0, depth, sizeX, sizeY, 1, textureFormat, dataType, data);
			glBindTexture(target, 0);
		}

		void GPUTexture::SetData(void* data, RendererAPI::GPUTextureCubemapFace face)
		{
			TF3D_ASSERT(type == RendererAPI::GPUTextureType_Cubemap, "GPUTexture::SetData(void* data, uint32_t depth) is only for 3D textures and texture arrays");
			TF3D_ASSERT(isSetupOnGPU, "Texture not setup on GPU (First call Setup)");
			TF3D_ASSERT(sizeX > 0 && sizeY > 0 && sizeZ > 0, "Texture sizes cannot be all 0");
			TF3D_ASSERT(data, "Data is null");
			UpdateInfo();
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glBindTexture(target, handle);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, internalFormat, sizeX, sizeY, 0, textureFormat, dataType, data);
			glBindTexture(target, 0);
		}

		ImTextureID GPUTexture::GetImGuiID()
		{
			return (ImTextureID)(handle);
		}

		void GPUTexture::UpdateInfo()
		{
			switch (type)
			{
			case TerraForge3D::RendererAPI::GPUTextureType_2D:
				target = GL_TEXTURE_2D;
				break;
			case TerraForge3D::RendererAPI::GPUTextureType_3D:
				target = GL_TEXTURE_3D;
				break;
			case TerraForge3D::RendererAPI::GPUTextureType_Cubemap:
				target = GL_TEXTURE_CUBE_MAP;
				break;
			case TerraForge3D::RendererAPI::GPUTextureType_Array:
				target = GL_TEXTURE_2D_ARRAY; 				
				break;
			default:
				TF3D_ASSERT(false, "Unknown GPUTextureType");
			}

			switch (format)
			{
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGB8I:
				internalFormat = GL_RGB8;
				dataType = GL_UNSIGNED_BYTE;
				textureFormat = GL_RGB;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGB16I:
				internalFormat = GL_RGB16;
				dataType = GL_UNSIGNED_SHORT;
				textureFormat = GL_RGB;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGB32F:
				internalFormat = GL_RGB32F;
				dataType = GL_FLOAT;
				textureFormat = GL_RGB;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGBA8I:
				internalFormat = GL_RGBA8;
				dataType = GL_UNSIGNED_BYTE;
				textureFormat = GL_RGBA;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGBA16I:
				internalFormat = GL_RGBA16;
				dataType = GL_UNSIGNED_SHORT;
				textureFormat = GL_RGBA;
				break;
			case TerraForge3D::RendererAPI::GPUTextureFormat_RGBA32F:
				internalFormat = GL_RGBA32F;
				textureFormat = GL_RGBA;
				dataType = GL_FLOAT;
				break;
			default:
				TF3D_ASSERT(false, "Unknown texture format");
			}
		}

	}

}