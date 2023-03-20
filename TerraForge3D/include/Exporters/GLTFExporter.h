#pragma once
#include <string>
#include <sstream>

#include <json.hpp>

class Mesh;
class BinaryFileWriter;

class GLTFExporter
{
public:
	GLTFExporter();
	~GLTFExporter();

	bool ExportGLTF(const std::string& path, const std::string& bin_path, Mesh* mesh, float* progress = nullptr);
	bool ExportGLB(const std::string& path, Mesh* mesh, float* progress = nullptr);

private:
	void PrepareMeta(Mesh* mesh, const std::string& bin_path);
	void WriteBinaryData(Mesh* mesh, BinaryFileWriter* writer, float* progress);

private:
	float m_Progress = 0.0f;
	char buffer[4096] = {};
	nlohmann::json meta_data;
};