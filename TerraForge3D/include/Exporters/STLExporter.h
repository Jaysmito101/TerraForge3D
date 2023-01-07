#pragma once
#include <string>
#include <sstream>

class Mesh;
class BinaryFileWriter;

class STLExporter
{
public:
	STLExporter();
	~STLExporter();

	bool ExportASCII(const std::string& path, Mesh* mesh, float* progress = nullptr);
	bool ExportBinary(const std::string& path, Mesh* mesh, float* progress = nullptr);

private:
	float m_Progress = 0.0f;
	char buffer[4096] = {};
};