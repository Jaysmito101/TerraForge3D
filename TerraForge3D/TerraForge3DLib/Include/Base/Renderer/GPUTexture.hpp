#pragma once
#include "Base/Core/Core.hpp"
#include "imgui/imgui.h"

namespace TerraForge3D
{

	namespace RendererAPI
	{
		enum GPUTextureFormat
		{
			GPUTextureFormat_RGB8I = 0,
			GPUTextureFormat_RGB16I,
			GPUTextureFormat_RGB32F,
			GPUTextureFormat_RGBA8I,
			GPUTextureFormat_RGBA16I,
			GPUTextureFormat_RGBA32F,
			GPUTextureFormat_Count
		};

		enum GPUTextureType
		{
			GPUTextureType_2D = 0,
			GPUTextureType_3D,
			GPUTextureType_Cubemap,
			GPUTextureType_Array,
			GPUTextureType_Count
		};

		/*
		* The faces of the cubemap texture
		*/
		enum GPUTextureCubemapFace
		{
			GPUTextureCubemapFace_PX = 0,
			GPUTextureCubemapFace_NX = 1,
			GPUTextureCubemapFace_PY = 2,
			GPUTextureCubemapFace_NY = 3,
			GPUTextureCubemapFace_PZ = 4,
			GPUTextureCubemapFace_NZ = 5
		};
		
		class GPUTexture
		{
		public:
			GPUTexture();
			virtual ~GPUTexture();

			/*
			* This sets up the GPU handle of the texture usng the native Backend (Vulkan or OpenGL)
			* This also allocates the necessaey storage.
			* 
			* Note: Make sure you set the proper type, format before this call also dont change them before calling Destroy
			*/
			virtual void Setup() = 0;

			/*
			* Destroys the GPU handle also frees the allocated resources and memory
			*/
			virtual void Destroy() = 0;

			/*
			* Set data of the texture
			* This data should be of correct type as set in type
			*/
			virtual void SetData(void* data) = 0; // For texture 2d
			virtual void SetData(void* data, uint32_t depth) = 0; // For texture 3d
			virtual void SetData(void* data, GPUTextureCubemapFace face) = 0; // For cubemap textures

			virtual void GetData(void* data) = 0; // Get the entire thing back

			/*
			* This returns a texture id (Gluint) if using the OpenGL backend
			* or returns a VkImage if using Vulkan backend
			*/
			virtual void GetHandle(void* handle) = 0; // Gluint for OpenGL, VkImage for vulkan

			/*
			* Get the ImTextureId for this texture
			*/
			virtual ImTextureID GetImGuiID() = 0;
			

			inline void SetSize(uint32_t x = 0, uint32_t y = 0, uint32_t z = 1) { sizeX = x; sizeY = y; sizeZ = z; }
			inline void SetType(GPUTextureType t) { type = t; }
			inline void SetFormat(GPUTextureFormat f) { format = f; }


		public:
			static GPUTexture* Create();

		public:
			GPUTextureType type = GPUTextureType_2D;
			GPUTextureFormat format = GPUTextureFormat_RGB8I;
			bool autoDestroy = true;
			bool isSetupOnGPU = false;
			uint32_t sizeX = 0, sizeY = 0, sizeZ = 1;
		};

	}

}
