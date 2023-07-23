#include "Exporters/ExrTextureExporter.h"
#include "Base/BinaryFileWriter.h"
#include "Utils/Utils.h"

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include "tinyexr/tinyexr.h"

#include <string>
#include <sstream>

ExrTextureExporter::ExrTextureExporter()
{
}

ExrTextureExporter::~ExrTextureExporter()
{
}

bool ExrTextureExporter::ExportHeightmap(const std::string& path, float* data, int bitDepth, int resolution, float* progress)
{
	if (bitDepth != 8 && bitDepth != 16 && bitDepth != 32) return false;
	if (!progress) progress = &m_Progress; *progress = 0.0f;
	if (bitDepth)
	{
		ShowMessageBox("Exporting to EXR with bit depth other than 32 or 16 is not supported yet!");
		return false;
	}
	auto res = SaveExr(path, resolution, 1, bitDepth, data);
	*progress = 1.0f;
	return res;
}

bool ExrTextureExporter::SaveExr(const std::string& path, int resolution, int channels_count, int bitDepth, float* ch0_ptr, float* ch1_ptr, float* ch2_ptr)
{
	EXRHeader header;
	InitEXRHeader(&header);
	
	EXRImage image;
	InitEXRImage(&image);

	image.num_channels = channels_count;

	float* image_ptr[3] = { ch0_ptr, ch1_ptr, ch2_ptr };

	image.images = (unsigned char**)image_ptr;
	image.width = resolution;
	image.height = resolution;

	header.num_channels = channels_count;
	header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * header.num_channels);

	if (channels_count == 1)
	{
		strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
	}
	else if (channels_count == 3)
	{
		strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
		strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
		strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';
	}
	else return false;


	header.pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
	header.requested_pixel_types = (int*)malloc(sizeof(int) * header.num_channels);

	for (int i = 0; i < header.num_channels; i++)
	{
		header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
		header.requested_pixel_types[i] = bitDepth == 8 ? TINYEXR_PIXELTYPE_UINT : (bitDepth == 16 ? TINYEXR_PIXELTYPE_HALF : TINYEXR_PIXELTYPE_FLOAT);
	}

	const char* err = nullptr;
	int ret = SaveEXRImageToFile(&image, &header, path.c_str(), &err);
	if (ret != TINYEXR_SUCCESS)
	{
		Log(fmt::format("Failed to save EXR image: {}", err));
		FreeEXRErrorMessage(err);
		return false;
	}

	free(header.channels);
	free(header.pixel_types);
	free(header.requested_pixel_types);
	return true;
}