#pragma once
#include <string>
#include <sstream>

class Mesh;

class ColladaExporter
{
public:
	ColladaExporter();
	~ColladaExporter();

	bool Export(const std::string& path, Mesh* mesh, float* progress = nullptr);

private:
	bool PrepareTexCoords(Mesh* mesh, std::stringstream& strm);
	bool PrepareVertices(Mesh* mesh, std::stringstream& strm);
	bool PrepareIndices(Mesh* mesh, std::stringstream& strm);

private:
	float m_Progress = 0.0f;
	char buffer[4096] = {};
};