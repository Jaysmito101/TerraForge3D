#include "Exporters/RawTextureExporter.h"
#include "Base/BinaryFileWriter.h"
#include "Utils/Utils.h"

#include <string>
#include <sstream>

RawTextureExporter::RawTextureExporter()
{
}

RawTextureExporter::~RawTextureExporter()
{
}

bool RawTextureExporter::ExportHeightmap(const std::string& path, float* data, int bitDepth, int resolution, float* progress)
{
	if (bitDepth != 8 && bitDepth != 16 && bitDepth != 32) return false;

	if (!progress) progress = &m_Progress; *progress = 0.0f;
	BinaryFileWriter writer(path);
	if (!writer.IsOpen()) return false;

	if (bitDepth == 32) // float export 
		writer.Write(data, resolution * resolution * sizeof(float));
	else
	{
		auto bufferSize = resolution * resolution * (bitDepth == 8 ? sizeof(unsigned char) : sizeof(unsigned short));
		auto buffer = new unsigned char[bufferSize];
		for (int i = 0; i < resolution * resolution; ++i)
		{
			auto clampedValue = std::clamp(data[i], 0.0f, 1.0f);
			if (bitDepth == 8) buffer[i] = (unsigned char)(clampedValue * 255.0f);
			else ((unsigned short*)buffer)[i] = (unsigned short)(clampedValue * 65535.0f);
			if(i % 100 == 0) *progress = ((float)i / (float)(resolution * resolution)) * 0.8f + 0.1f;
		}
		*progress = 0.9f;
		writer.Write(buffer, bufferSize);
		delete[] buffer;
		*progress = 1.0f;
	}
	*progress = 1.0f;
	return true;
}
