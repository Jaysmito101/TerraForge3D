#include "Exporters/WebpTextureExporter.h"
#include "Base/BinaryFileWriter.h"
#include "Utils/Utils.h"

#include <webp/encode.h>

#include <string>
#include <sstream>

WebpTextureExporter::WebpTextureExporter()
{
}

WebpTextureExporter::~WebpTextureExporter()
{
}

bool WebpTextureExporter::ExportHeightmap(const std::string& path, float* data, int bitDepth, int resolution, float* progress)
{
	// ShowMessageBox("This feature is currently not supported!");
	if (bitDepth == 32 || bitDepth == 16)
	{
		ShowMessageBox("Exporting 32-bit and 16-bit heightmaps to WebP is currently not supported!");
		return false;
	}

	if (!progress) progress = &m_Progress; *progress = 0.0f;
	
	BinaryFileWriter writer(path);
	if (!writer.IsOpen()) return false;
	
	auto buffeSize = resolution * resolution * sizeof(unsigned char);
	auto buffer = new unsigned char[buffeSize * 3];
	for (int i = 0; i < resolution * resolution; ++i)
	{
		auto clampedValue = std::clamp(data[i], 0.0f, 1.0f);
		buffer[i * 3] = buffer[i * 3 + 1] = buffer[i * 3 + 2] = (unsigned char)(clampedValue * 255.0f);
		if (i % 100 == 0) *progress = ((float)i / (float)(resolution * resolution)) * 0.8f + 0.1f;
	}

	WebPConfig config;
	WebPConfigInit(&config);
	config.lossless = 1;
	
	uint8_t* webpData = nullptr;
	size_t webpSize = WebPEncodeLosslessRGB(buffer, resolution, resolution, resolution * 3, &webpData);

	writer.Write(webpData, webpSize);

	free(webpData);
	delete[] buffer;

	*progress = 0.9f;


	return true;
}