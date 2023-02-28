#include "Exporters/GLTFExporter.h"
#include "Base/BinaryFileWriter.h"
#include "Utils/Utils.h"

#include <string>
#include <sstream>

GLTFExporter::GLTFExporter()
{
}

GLTFExporter::~GLTFExporter()
{
}

bool GLTFExporter::ExportGLB(const std::string& path, Mesh* mesh, float* progress)
{
	if (!progress) progress = &m_Progress; *progress = 0.0f;
	BinaryFileWriter writer(path);
	if (!writer.IsOpen()) return false;
	std::cout << "GLB Export is yet to be implemented!\n";
	*progress = 1.0f;
	return true;
}


bool GLTFExporter::ExportGLTF(const std::string& path, const std::string& bin_path, Mesh* mesh, float* progress)
{
	if (!progress) progress = &m_Progress; *progress = 0.0f;

	this->PrepareMeta(mesh, bin_path);

	std::ofstream out_file; out_file.open(path);
	if (!out_file.is_open()) return false;
	out_file << meta_data.dump(4);
	out_file.close();

	BinaryFileWriter writer(bin_path);
	if (!writer.IsOpen()) return false;
	this->WriteBinaryData(mesh, &writer, progress);
	
	*progress = 1.0f;
	return true;
}

void GLTFExporter::PrepareMeta(Mesh* mesh, const std::string& bin_path)
{
	nlohmann::json gltf_js, tmp_js, tmp2_js, tmp3_js;

	tmp_js["generator"] = "TerraForge3D Gen 2";
	tmp_js["version"] = "2.0";
	gltf_js["asset"] = tmp_js; tmp_js = nlohmann::json();

	gltf_js["scene"] = 0;

	tmp2_js["name"] = "Scene";
	tmp2_js["nodes"] = { 0 };
	tmp_js.push_back(tmp2_js); tmp2_js = nlohmann::json();
	gltf_js["scenes"] = tmp_js; tmp_js = nlohmann::json();

	tmp2_js["mesh"] = 0;
	tmp2_js["name"] = "MainOBJ";
	tmp_js.push_back(tmp2_js); tmp2_js = nlohmann::json();
	gltf_js["nodes"] = tmp_js; tmp_js = nlohmann::json();

	tmp3_js["POSITION"] = 0;
	tmp3_js["NORMAL"] = 1;
	tmp3_js["TEXCOORD_0"] = 2;
	tmp2_js["attributes"] = tmp3_js; tmp3_js = nlohmann::json();
	tmp2_js["indices"] = 3;
	tmp_js["primitives"] = nlohmann::json();
	tmp_js["primitives"].push_back(tmp2_js); tmp2_js = nlohmann::json();
	tmp_js["name"] = "MainOBJ";
	gltf_js["meshes"] = nlohmann::json();
	gltf_js["meshes"].push_back(tmp_js); tmp_js = nlohmann::json();

	gltf_js["accessors"] = nlohmann::json();

	tmp_js["bufferView"] = 0;
	tmp_js["componentType"] = 5126;
	tmp_js["count"] = mesh->GetVertexCount();
	tmp_js["type"] = "VEC3";
	gltf_js["accessors"].push_back(tmp_js); tmp_js = nlohmann::json();

	tmp_js["bufferView"] = 1;
	tmp_js["componentType"] = 5126;
	tmp_js["count"] = mesh->GetVertexCount();
	tmp_js["type"] = "VEC3";
	gltf_js["accessors"].push_back(tmp_js); tmp_js = nlohmann::json();

	tmp_js["bufferView"] = 2;
	tmp_js["componentType"] = 5126;
	tmp_js["count"] = mesh->GetVertexCount();
	tmp_js["type"] = "VEC2";
	gltf_js["accessors"].push_back(tmp_js); tmp_js = nlohmann::json();

	tmp_js["bufferView"] = 3;
	tmp_js["componentType"] = 5125;
	tmp_js["count"] = mesh->GetIndexCount();
	tmp_js["type"] = "SCALAR";
	gltf_js["accessors"].push_back(tmp_js); tmp_js = nlohmann::json();

	gltf_js["bufferViews"] = nlohmann::json();

	auto byte_offset = 0u;
	tmp_js["buffer"] = 0;
	tmp_js["byteLength"] = sizeof(float) * 3 * mesh->GetVertexCount();
	tmp_js["byteOffset"] = byte_offset;
	gltf_js["bufferViews"].push_back(tmp_js); tmp_js = nlohmann::json();
	byte_offset += sizeof(float) * 3 * mesh->GetVertexCount();

	tmp_js["buffer"] = 0;
	tmp_js["byteLength"] = sizeof(float) * 3 * mesh->GetVertexCount();
	tmp_js["byteOffset"] = byte_offset;
	gltf_js["bufferViews"].push_back(tmp_js); tmp_js = nlohmann::json();
	byte_offset += sizeof(float) * 3 * mesh->GetVertexCount();

	tmp_js["buffer"] = 0;
	tmp_js["byteLength"] = sizeof(float) * 2 * mesh->GetVertexCount();
	tmp_js["byteOffset"] = byte_offset;
	gltf_js["bufferViews"].push_back(tmp_js); tmp_js = nlohmann::json();
	byte_offset += sizeof(float) * 2 * mesh->GetVertexCount();

	tmp_js["buffer"] = 0;
	tmp_js["byteLength"] = sizeof(uint32_t) * mesh->GetVertexCount();
	tmp_js["byteOffset"] = byte_offset;
	gltf_js["bufferViews"].push_back(tmp_js); tmp_js = nlohmann::json();
	byte_offset += sizeof(uint32_t) * mesh->GetIndexCount();

	gltf_js["buffers"] = nlohmann::json();

	tmp_js["byteLength"] = byte_offset;
	tmp_js["uri"] = bin_path;
	gltf_js["buffers"].push_back(tmp_js); tmp_js = nlohmann::json();

	meta_data = gltf_js;
}

void GLTFExporter::WriteBinaryData(Mesh* mesh, BinaryFileWriter* writer, float* progress)
{
	for (auto i = 0; i < mesh->GetVertexCount(); i++)
	{
		const auto& pos = mesh->GetPosition(i);
		float f_buffer[3] = { pos.x, pos.y, pos.z };
		writer->Write(f_buffer, sizeof(float) * 3);
	}
	*progress = 0.4f;

	for (auto i = 0; i < mesh->GetVertexCount(); i++)
	{
		const auto& normal = mesh->GetNormal(i);
		float f_buffer[3] = { normal.x, normal.y, normal.z };
		writer->Write(f_buffer, sizeof(float) * 3);
	}
	*progress = 0.5f;

	for (auto i = 0; i < mesh->GetVertexCount(); i++)
	{
		const auto& texCoord = mesh->GetTexCoord(i);
		float f_buffer[2] = { texCoord.x, texCoord.y };
		writer->Write(f_buffer, sizeof(float) * 2);
	}
	*progress = 0.6f;

	for (auto i = 0; i < mesh->GetFaceCount(); i++)
	{
		const auto& face = mesh->GetFace(i);
		for (int j = 0; j < 3; j++)
		{
			uint32_t index = (uint32_t)face.indices[j];
			writer->Write(&index, sizeof(uint32_t));
		}
	}
	*progress = 0.9f;
}
