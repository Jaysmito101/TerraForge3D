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
					stbi_image_free(rawTextureData);
					rawTextureData = nullptr;
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
			if (storageMode == TextureStorageMode_GPUOnly)
			{
				stbi_image_free(rawTextureData);
				rawTextureData = nullptr;
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

	void Texture2D::GetPixel(float u, float v, float* pr, float* pg, float* pb, float* pa)
	{
		TF3D_ASSERT(storageMode != TextureStorageMode_GPUOnly, "Cannot get pixel values with texture storage mode GPUOnly"); // Will soon be updated to support this as it will be needed for things like mouse picking

		uint32_t x = static_cast<uint32_t>(u * (width - 1));
		uint32_t y = static_cast<uint32_t>(v * (height - 1));
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
		uint32_t bytesPerPixel = channels * dataType;

		switch (dataType)
		{
		case TextureDataType_8UI:
		{
			uint8_t* pixel = &reinterpret_cast<uint8_t*>(rawTextureData)[width * y * bytesPerPixel + x * bytesPerPixel];
			r = static_cast<float>(pixel[0] / 255.0f);
			g = static_cast<float>(pixel[1] / 255.0f);
			b = static_cast<float>(pixel[2] / 255.0f);
			if(channels == TextureChannels_RGBA)
				a = static_cast<float>(pixel[3] / 255.0f);
			break;
		}
		case TextureDataType_16UI:
		{
			uint16_t* pixel = &reinterpret_cast<uint16_t*>(rawTextureData)[width * y * bytesPerPixel + x * bytesPerPixel];
			r = static_cast<float>(pixel[0] / 65535.0f);
			g = static_cast<float>(pixel[1] / 65535.0f);
			b = static_cast<float>(pixel[2] / 65535.0f);
			if (channels == TextureChannels_RGBA)
				a = static_cast<float>(pixel[3] / 65535.0f);
			break;
		}
		case TextureDataType_32F:
		{
			float* pixel = &reinterpret_cast<float*>(rawTextureData)[width * y * bytesPerPixel + x * bytesPerPixel];
			r = pixel[0];
			g = pixel[1];
			b = pixel[2];
			if (channels == TextureChannels_RGBA)
				a = pixel[3];
			break;
		}
		default:
		{
			TF3D_ASSERT(false, "Unknown Texture Data Type");
			break;
		}
		}
		if (pr)
			*pr = r;
		if (pg)
			*pg = g;
		if (pb)
			*pb = b;
		if (pa)
			*pa = a;
	}

	void Texture2D::SetPixel(float u, float v, float r, float g, float b, float a)
	{
		TF3D_ASSERT(storageMode != TextureStorageMode_GPUOnly, "Cannot get pixel values with texture storage mode GPUOnly"); // Will soon be updated to support this as it will be needed for things like mouse picking

		uint32_t x = static_cast<uint32_t>(u * (width - 1));
		uint32_t y = static_cast<uint32_t>(v * (height - 1));
		uint32_t bytesPerPixel = channels * dataType;

		switch (dataType)
		{
		case TextureDataType_8UI:
		{
			uint8_t* pixel = &reinterpret_cast<uint8_t*>(rawTextureData)[width * y * bytesPerPixel + x * bytesPerPixel];
			pixel[0] = static_cast<uint8_t>(r * 255);
			pixel[1] = static_cast<uint8_t>(g * 255);
			pixel[2] = static_cast<uint8_t>(b * 255);
			if (channels == TextureChannels_RGBA)
				pixel[3] = static_cast<uint8_t>(a * 255);
			break;
		}
		case TextureDataType_16UI:
		{
			uint16_t* pixel = &reinterpret_cast<uint16_t*>(rawTextureData)[width * y * bytesPerPixel + x * bytesPerPixel];
			pixel[0] = static_cast<uint16_t>(r * 255);
			pixel[1] = static_cast<uint16_t>(g * 255);
			pixel[2] = static_cast<uint16_t>(b * 255);
			if (channels == TextureChannels_RGBA)
				pixel[3] = static_cast<uint16_t>(a * 255);
			break;
		}
		case TextureDataType_32F:
		{
			float* pixel = &reinterpret_cast<float*>(rawTextureData)[width * y * bytesPerPixel + x * bytesPerPixel];
			pixel[0] = r;
			pixel[1] = g;
			pixel[2] = b;
			if (channels == TextureChannels_RGBA)
				pixel[3] = a;
			break;
		}
		default:
		{
			TF3D_ASSERT(false, "Unknown Texture Data Type");
			break;
		}
		}

		if (storageMode == TextureStorageMode_CPUAndGPU || storageMode == TextureStorageMode_GPUOnly)
		{
			gpuTextureHandle->SetData(rawTextureData);
		}
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