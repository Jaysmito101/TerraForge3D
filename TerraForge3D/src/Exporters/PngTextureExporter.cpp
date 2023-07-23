#include "Exporters/PngTextureExporter.h"
#include "Base/BinaryFileWriter.h"
#include "Utils/Utils.h"

#include "stb/stb_image_write.h"

#include <string>
#include <sstream>

PngTextureExporter::PngTextureExporter()
{
}

PngTextureExporter::~PngTextureExporter()
{
}

bool PngTextureExporter::ExportHeightmap(const std::string& path, float* data, int bitDepth, int resolution, float* progress)
{
	if (bitDepth != 8 && bitDepth != 16 && bitDepth != 32) return false;
	if (!progress) progress = &m_Progress; *progress = 0.0f;

	if (bitDepth == 32)
	{
		// TODO
		ShowMessageBox("PngTextureExporter::ExportHeightmap: 32-bit PNG export is not supported yet.");
	}
	else if (bitDepth == 16)
	{
		// TODO
		ShowMessageBox("PngTextureExporter::ExportHeightmap: 16-bit PNG export is not supported yet.");
	}
	else if (bitDepth == 8)
	{
		auto data8 = new uint8_t[resolution * resolution];
		for (int i = 0; i < resolution * resolution; i++)
			data8[i] = (uint8_t)(data[i] * 255.0f);
		stbi_write_png(path.c_str(), resolution, resolution, 1, data8, resolution * sizeof(uint8_t));
		delete[] data8;
	}

	*progress = 1.0f;

	return true;
}

