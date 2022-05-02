#include "Base/DS/Texture2D.hpp"
#include "Base/Renderer/GPUTexture.hpp"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image_resize.h"

namespace TerraForge3D
{



	Texture2D::Texture2D(bool autoDestory, TextureStorageMode storageMode)
	{
		this->autoDestory = autoDestory;
		this->storageMode = storageMode;

		gpuTextureHandle = RendererAPI::GPUTexture::Create();
		gpuTextureHandle->SetType(RendererAPI::GPUTextureType_2D);
		gpuTextureHandle->autoDestroy = false;
	}

	Texture2D::~Texture2D()
	{
		if (autoDestory && isLoaded)
			Destroy();

		if (gpuTextureHandle)
			delete gpuTextureHandle;
	}

	bool Texture2D::LoadFromFile(std::string filepath, TextureChannels channels, TextureDataType dataType)
	{
		TF3D_ASSERT(!isLoaded, "Texture already Loaded");
		TF3D_ASSERT(!gpuTextureHandle->isSetupOnGPU, "GPU Handle not cleared");

		if (dataType == TextureDataType_Auto)
		{
			dataType = TextureDataType_8UI;
			if (stbi_is_16_bit(filepath.c_str()))
				dataType = TextureDataType_16UI;
			if (stbi_is_hdr(filepath.c_str()))
				dataType = TextureDataType_32F;
		}

		int w, h, ch;
		ch = static_cast<int>(channels);

		switch (dataType)
		{
			case TextureDataType_8UI:
			{
				rawTextureData = stbi_load(filepath.c_str(), &w, &h, &ch, ch);
				break;
			}
			case TextureDataType_16UI:
			{
				rawTextureData = stbi_load_16(filepath.c_str(), &w, &h, &ch, ch);
				break;
			}
			case TextureDataType_32F:
			{
				rawTextureData = stbi_loadf(filepath.c_str(), &w, &h, &ch, ch);
				break;
			}
			default:
			{
				TF3D_ASSERT(false, "Unknown Texture Data Type");
				break;
			}
		}

		if (!rawTextureData)
		{
			TF3D_LOG_ERROR("Failed to load texture {0}", filepath);
			return false;
		}
		else
		{
#ifdef TF3D_DEBUG
			TF3D_LOG("Loaded texture from file {0}, width : {1}, height : {2}, channels : {3}", filepath, w, h, ch);
#endif
			this->width = static_cast<uint32_t>(w);
			this->height = static_cast<uint32_t>(h);
			this->channels = static_cast<TextureChannels>(ch);

			switch (storageMode)
			{
				case TextureStorageMode_CPUAndGPU:
				{
					loadedOnCPU = true;
					loadedOnGPU = true;
					isCPUDataShared = false;
					SetUpGPUTextureHandle();
					gpuTextureHandle->SetSize(width, height);
					gpuTextureHandle->Setup();
					gpuTextureHandle->SetData(rawTextureData);
					break;
				}
				case TextureStorageMode_CPUOnly:
				{
					loadedOnCPU = true;
					loadedOnGPU = false;
					isCPUDataShared = false;
					break;
				}
				case TextureStorageMode_GPUOnly:
				{
					loadedOnCPU = false;
					loadedOnGPU = true;
					isCPUDataShared = false;
					SetUpGPUTextureHandle();
					gpuTextureHandle->SetSize(width, height);
					gpuTextureHandle->SetData(rawTextureData);
					break;
				}
				default:
				{
					TF3D_ASSERT(false, "Unknown Texture Storage Mode");
					break;
				}
			}

			isLoaded = true;
		}
		return true;
	}

	bool Texture2D::LoadFromMemory(void* rawData, uint32_t w, uint32_t h, TextureChannels ch, TextureDataType dt, bool isRawData, bool isSharedData)
	{
		TF3D_ASSERT(ch != TextureChannels_Auto, "Channels must be RGB or RGBA");
		TF3D_ASSERT(dt != TextureDataType_Auto, "Channels must be 8UI or 16UI or 32F");
		TF3D_ASSERT(w != 0 && h != 0, "Width or height cannot be 0");

		if (isRawData)
		{
			channels = ch;
			dataType = dt;
			width = w;
			height = h;
			if (storageMode == TextureStorageMode_CPUOnly || storageMode == TextureStorageMode_CPUAndGPU)
			{
				if (isSharedData)
				{
					rawTextureData = rawData;
					isCPUDataShared = true;
				}
				else
				{
					size_t size = w * h * ch;

					rawTextureData = malloc(size);
					memcpy(rawTextureData, rawData, size);
					isCPUDataShared = false;
				}
				loadedOnCPU = true;
			}
			if (storageMode == TextureStorageMode_GPUOnly || storageMode == TextureStorageMode_CPUAndGPU)
			{
				SetUpGPUTextureHandle();
				gpuTextureHandle->sizeX = w;
				gpuTextureHandle->sizeY = h;
				gpuTextureHandle->Setup();
				gpuTextureHandle->SetData(rawData);
			}
			isLoaded = true;
		}
		else
		{
			rawTextureData = nullptr;
			int wd, he, cc;
			dataType = dt;
			switch (dataType)
			{
			case TextureDataType_8UI:
				rawTextureData = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(rawData), w * h * ch * TextureDataType_8UI, &wd, &he, &cc, ch);
				break;
			case TextureDataType_16UI:
				rawTextureData = stbi_load_16_from_memory(reinterpret_cast<stbi_uc*>(rawData), w * h * ch * TextureDataType_16UI, &wd, &he, &cc, ch);
				break;
			case TextureDataType_32F:
				rawTextureData = stbi_load_16_from_memory(reinterpret_cast<stbi_uc*>(rawData), w * h * ch * TextureDataType_32F, &wd, &he, &cc, ch);
				break;
			default:
				TF3D_ASSERT(false, "Unknown Texture Data Type");
			}
			if (!rawTextureData)
				return false;
			width = wd;
			height = he;
			channels = static_cast<TextureChannels>(cc);
			if (storageMode == TextureStorageMode_CPUOnly || storageMode == TextureStorageMode_CPUAndGPU)
			{
				isLoaded = true;
				loadedOnCPU = true;
			}
			if (storageMode == TextureStorageMode_GPUOnly || storageMode == TextureStorageMode_CPUAndGPU)
			{
				SetUpGPUTextureHandle();
				gpuTextureHandle->sizeX = w;
				gpuTextureHandle->sizeZ = h;
				gpuTextureHandle->Setup();
				gpuTextureHandle->SetData(rawTextureData);
				isLoaded = true;
				loadedOnGPU = true;
			}
		}

		return true;
	}

	void Texture2D::Destroy()
	{
		TF3D_ASSERT(isLoaded, "Texture not yet loaded. Cannot call destroy");

		if (loadedOnGPU)
		{
			TF3D_ASSERT(gpuTextureHandle, "GPU Texture handle not yet setup");
			gpuTextureHandle->Destroy();
		}

		if (loadedOnCPU)
		{
			TF3D_ASSERT(rawTextureData, "Raw Texture data on CPU was null");
			if (!isCPUDataShared)
			{
				stbi_image_free(rawTextureData);
			}
		}

		isLoaded = false;
		loadedOnCPU = false;
		loadedOnGPU = false;
		isCPUDataShared = false;
		rawTextureData = nullptr;
	}

	inline void Texture2D::SetUpGPUTextureHandle()
	{
		if (channels == TextureChannels_RGB)
		{
			switch (dataType)
			{
				case TextureDataType_8UI:
				{
					gpuTextureHandle->SetFormat(RendererAPI::GPUTextureFormat_RGB8I);
					break;
				}
				case TextureDataType_16UI:
				{
					gpuTextureHandle->SetFormat(RendererAPI::GPUTextureFormat_RGB16I);
					break;
				}
				case TextureDataType_32F:
				{
					gpuTextureHandle->SetFormat(RendererAPI::GPUTextureFormat_RGB32F);
					break;
				}
				default:
				{
					TF3D_ASSERT(false, "Unknown Texture Data Type");
					break;
				}
			}
		}
		else if (channels == TextureChannels_RGBA)
		{
			switch (dataType)
			{
				case TextureDataType_8UI:
				{
					gpuTextureHandle->SetFormat(RendererAPI::GPUTextureFormat_RGBA8I);
					break;
				}
				case TextureDataType_16UI:
				{
					gpuTextureHandle->SetFormat(RendererAPI::GPUTextureFormat_RGBA16I);
					break;
				}
				case TextureDataType_32F:
				{
					gpuTextureHandle->SetFormat(RendererAPI::GPUTextureFormat_RGBA32F);
					break;
				}
				default:
				{
					TF3D_ASSERT(false, "Unknown Texture Data Type");
					break;
				}
			}
		}
		else
		{
			TF3D_ASSERT(false, "Unknown Texture Channel Format");
		}
	}

}