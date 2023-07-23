#pragma once
#include <string>
#include <sstream>


class PngTextureExporter
{
public:
	PngTextureExporter();
	~PngTextureExporter();

	bool ExportHeightmap(const std::string& path, float* data, int bitDepth, int resolution, float* progress = nullptr);

private:

private:
	float m_Progress = 0.0f;
	char buffer[4096] = {};
};