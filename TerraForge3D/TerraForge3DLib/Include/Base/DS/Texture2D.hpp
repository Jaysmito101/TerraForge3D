#pragma once
#include "Base/DS/Texture.hpp"
#include "Base/Renderer/GPUTexture.hpp"

#include "imgui/imgui.h"


namespace TerraForge3D
{
	class Texture2D
	{
	public:
		Texture2D(bool autoDestory = false, TextureStorageMode storageMode = TextureStorageMode_Default);
		~Texture2D();

		bool LoadFromFile(std::string filepath, TextureChannels channels = TextureChannels_Auto, TextureDataType dataType = TextureDataType_Auto);
		bool LoadFromMemory(void* rawData, uint32_t width, uint32_t height, TextureChannels channels, TextureDataType dataType, bool isRawData, bool isSharedData);

		void Destroy();


		inline uint32_t GetWidth() { return width; }
		inline uint32_t GetHeight() { return height; }
		inline std::pair<uint32_t, uint32_t> GetSize() { return { width, height }; } // For something like : auto [w, h] = texture->GetSize();
		inline bool IsLoaded() { return isLoaded; }
		inline bool IsLoadedOnGPU() { return loadedOnGPU; }
		inline bool IsLoadedOnCPU() { return loadedOnCPU; }
		inline bool IsCPUDataShared() { return isCPUDataShared; }
		inline TextureChannels GetChannels() { return channels; }
		inline TextureDataType GetDataType() { return dataType; }
		inline size_t GetDataSize() { return dataSize; }
		inline void* GetCPUDataPTR() { TF3D_ASSERT(loadedOnCPU, "Texture not loaded on CPU"); return rawTextureData; }
		inline RendererAPI::GPUTexture* GetGPUHandle() { TF3D_ASSERT(gpuTextureHandle, "GPU Texture handle is null"); return gpuTextureHandle; }
		inline void SetStorageMode(TextureStorageMode storageMode) { this->storageMode = storageMode; };
		inline ImTextureID GetImGuiID() { /* TF3D_ASSERT(loadedOnGPU, "Cannot Get ImTextureID of texture not loaded on GPU"); */ return gpuTextureHandle->GetImGuiID(); }

	private:
		void SetUpGPUTextureHandle();





	private:
		TextureStorageMode storageMode = TextureStorageMode_Default;
		RendererAPI::GPUTexture* gpuTextureHandle = nullptr;
		void* rawTextureData = nullptr;
		size_t dataSize = 0;
		bool isLoaded = false;
		bool loadedOnGPU = false;
		bool loadedOnCPU = false;
		bool autoDestory = true;
		bool isCPUDataShared = false;
		uint32_t width = 0, height = 0;
		TextureChannels channels = TextureChannels_Auto;
		TextureDataType dataType = TextureDataType_8UI;
	};

}
