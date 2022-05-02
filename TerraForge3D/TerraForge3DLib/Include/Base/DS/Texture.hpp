#pragma once
#include "Base/Core/Core.hpp"

namespace TerraForge3D
{

	enum TextureStorageMode
	{
		TextureStorageMode_CPUOnly = 0,
		TextureStorageMode_GPUOnly,
		TextureStorageMode_CPUAndGPU,
		TextureStorageMode_Default = TextureStorageMode_CPUAndGPU
	};


	enum TextureChannels
	{
		TextureChannels_Auto = 0,
		TextureChannels_RGB = 3,
		TextureChannels_RGBA = 4
	};

	enum TextureDataType
	{
		TextureDataType_Auto = 0,
		TextureDataType_8UI = sizeof(uint8_t),
		TextureDataType_16UI = sizeof(uint16_t),
		TextureDataType_32F = sizeof(float)
	};

	inline std::string ToString(TextureChannels channels)
	{
		switch (channels)
		{
			case TextureChannels_RGB:	return "RGB";
			case TextureChannels_RGBA:	return "RGBA";
			case TextureChannels_Auto:	return "Auto";
			default:					return "Unknown";
		}
	}

	inline std::string ToString(TextureDataType dataType)
	{
		switch (dataType)
		{
			case TextureDataType_Auto:		return "Auto";
			case TextureDataType_8UI:		return "8UI";
			case TextureDataType_16UI:		return "16UI";
			case TextureDataType_32F:		return "32F";
			default:						return "Unnown";
		}
	}
}