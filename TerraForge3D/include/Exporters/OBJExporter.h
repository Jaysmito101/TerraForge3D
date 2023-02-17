#pragma once
#include <string>
#include <sstream>

class Mesh;

class OBJExporter
{
public:
	OBJExporter();
	~OBJExporter();

	bool Export(const std::string& path, Mesh* mesh, float* progress = nullptr);

private:
	bool WriteHeader(std::stringstream& out_strm, Mesh* mesh, float* progress);
	bool WriteVertices(std::stringstream& out_strm, Mesh* mesh, float* progress);
	bool WriteNormals(std::stringstream& out_strm, Mesh* mesh, float* progress);
	bool WriteTextureCoordinates(std::stringstream& out_strm, Mesh* mesh, float* progress);
	bool WriteFaces(std::stringstream& out_strm, Mesh* mesh, float* progress);

private:
	float m_Progress = 0.0f;
	char buffer[4096] = {};
};