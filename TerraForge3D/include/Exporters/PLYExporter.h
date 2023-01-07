#pragma once
#include <string>
#include <sstream>

class Mesh;
class BinaryFileWriter;

class PLYExporter
{
public:
	PLYExporter();
	~PLYExporter();

	bool ExportASCII(const std::string& path, Mesh* mesh, float* progress = nullptr);
	bool ExportBinary(const std::string& path, Mesh* mesh, float* progress = nullptr);

private:
	bool WriteHeader(BinaryFileWriter* writer, const std::string& format, Mesh* mesh);

private:
	float m_Progress = 0.0f;
	char buffer[4096] = {};
};