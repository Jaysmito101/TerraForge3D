#include "Exporters/ExportManager.h"
#include "Data/ApplicationState.h"
#include "Base/Base.h"

#include "Data/Serializer.h"

#include <sstream>
#include <fstream>

#include <iostream>
#include <string>
#include <stdio.h>
#include <filesystem>
#include <time.h>

// From : https://stackoverflow.com/a/10467633/14911094
// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

	return buf;
}

void ExportManager::ExportMeshOBJ(std::string path, Mesh* mesh)
{
	this->exportProgress = 0.01f;
	for (int i = path.size() - 3; i < path.size(); i++) if (!(path[i] >= 65 && path[i] < 65 + 26) && !(path[i] >= 97 && path[i] < 97 + 26) && !(path[i] >= 48 && path[i] < 48 + 10)) path[i] = '_';
	if (path.find(".obj") >= path.size() || path.find(".obj") < 0) path = path + ".obj";
	while (appState->states.remeshing);
	appState->states.pauseUpdation = true;

	this->worker_th = std::thread([path, mesh, this]() -> void {

		for (int i = 0; i < mesh->vertexCount; i++)
		{
			const auto& vert = mesh->vert[i];
			auto tmp = 0.0f; appState->mainMap.currentTileDataLayers[0]->GetPixelF(vert.texCoord.x, vert.texCoord.y, &tmp);
			mesh->vert[i].position  = vert.position + vert.normal * tmp;
		}
		appState->states.pauseUpdation = false;
		mesh->RecalculateNormals();
		objExporter.Export(path, mesh, &this->exportProgress);

		this->exportProgress = 1.1f;

		delete mesh;
		});
	this->worker_th.detach();
}

void ExportManager::ExportMeshSTLBinary(std::string path, Mesh* mesh)
{
	this->exportProgress = 0.01f;
	for (int i = path.size() - 3; i < path.size(); i++) if (!(path[i] >= 65 && path[i] < 65 + 26) && !(path[i] >= 97 && path[i] < 97 + 26) && !(path[i] >= 48 && path[i] < 48 + 10)) path[i] = '_';
	if (path.find(".stl") >= path.size() || path.find(".stl") < 0) path = path + ".stl";
	while (appState->states.remeshing);
	appState->states.pauseUpdation = true;

	this->worker_th = std::thread([path, mesh, this]() -> void {
		for (int i = 0; i < mesh->vertexCount; i++)
		{
			const auto& vert = mesh->vert[i];
			auto tmp = 0.0f; appState->mainMap.currentTileDataLayers[0]->GetPixelF(vert.texCoord.x, vert.texCoord.y, &tmp);
			mesh->vert[i].position = vert.position + vert.normal * tmp;
		}
		appState->states.pauseUpdation = false;
		mesh->RecalculateNormals();
		stlExporter.ExportBinary(path, mesh, &this->exportProgress);
		this->exportProgress = 1.1f;
		delete mesh;
		});
	this->worker_th.detach();
}

void ExportManager::ExportMeshSTLASCII(std::string path, Mesh* mesh)
{
	this->exportProgress = 0.01f;
	for (int i = path.size() - 3; i < path.size(); i++) if (!(path[i] >= 65 && path[i] < 65 + 26) && !(path[i] >= 97 && path[i] < 97 + 26) && !(path[i] >= 48 && path[i] < 48 + 10)) path[i] = '_';
	if (path.find(".stl") >= path.size() || path.find(".stl") < 0) path = path + ".stl";
	while (appState->states.remeshing);
	appState->states.pauseUpdation = true;

	this->worker_th = std::thread([path, mesh, this]() -> void {
		for (int i = 0; i < mesh->vertexCount; i++)
		{
			const auto& vert = mesh->vert[i];
			auto tmp = 0.0f; appState->mainMap.currentTileDataLayers[0]->GetPixelF(vert.texCoord.x, vert.texCoord.y, &tmp);
			mesh->vert[i].position = vert.position + vert.normal * tmp;
		}
		appState->states.pauseUpdation = false;
		mesh->RecalculateNormals();
		stlExporter.ExportASCII(path, mesh, &this->exportProgress);
		this->exportProgress = 1.1f;
		delete mesh;
		});
	this->worker_th.detach();
}

void ExportManager::ExportMeshPLYASCII(std::string path, Mesh* mesh)
{
	this->exportProgress = 0.01f;
	for (int i = path.size() - 3; i < path.size(); i++) if (!(path[i] >= 65 && path[i] < 65 + 26) && !(path[i] >= 97 && path[i] < 97 + 26) && !(path[i] >= 48 && path[i] < 48 + 10)) path[i] = '_';
	if (path.find(".ply") >= path.size() || path.find(".ply") < 0) path = path + ".ply";
	while (appState->states.remeshing);
	appState->states.pauseUpdation = true;

	this->worker_th = std::thread([path, mesh, this]() -> void {
		for (int i = 0; i < mesh->vertexCount; i++)
		{
			const auto& vert = mesh->vert[i];
			auto tmp = 0.0f; appState->mainMap.currentTileDataLayers[0]->GetPixelF(vert.texCoord.x, vert.texCoord.y, &tmp);
			mesh->vert[i].position = vert.position + vert.normal * tmp;
		}
		appState->states.pauseUpdation = false;
		mesh->RecalculateNormals();
		plyExporter.ExportASCII(path, mesh, &this->exportProgress);
		this->exportProgress = 1.1f;
		delete mesh;
		});
	this->worker_th.detach();
}

void ExportManager::ExportMeshPLYBinary(std::string path, Mesh* mesh)
{
	this->exportProgress = 0.01f;
	for (int i = path.size() - 3; i < path.size(); i++) if (!(path[i] >= 65 && path[i] < 65 + 26) && !(path[i] >= 97 && path[i] < 97 + 26) && !(path[i] >= 48 && path[i] < 48 + 10)) path[i] = '_';
	if (path.find(".ply") >= path.size() || path.find(".ply") < 0) path = path + ".ply";
	while (appState->states.remeshing);
	appState->states.pauseUpdation = true;

	this->worker_th = std::thread([path, mesh, this]() -> void {
		for (int i = 0; i < mesh->vertexCount; i++)
		{
			const auto& vert = mesh->vert[i];
			auto tmp = 0.0f; appState->mainMap.currentTileDataLayers[0]->GetPixelF(vert.texCoord.x, vert.texCoord.y, &tmp);
			mesh->vert[i].position = vert.position + vert.normal * tmp;
		}
		appState->states.pauseUpdation = false;
		mesh->RecalculateNormals();
		plyExporter.ExportBinary(path, mesh, &this->exportProgress);
		this->exportProgress = 1.1f;
		delete mesh;
		});
	this->worker_th.detach();
}

void ExportManager::ExportMeshCollada(std::string path, Mesh* mesh)
{
	this->exportProgress = 0.01f;
	for (int i = path.size() - 3; i < path.size(); i++) if (!(path[i] >= 65 && path[i] < 65 + 26) && !(path[i] >= 97 && path[i] < 97 + 26) && !(path[i] >= 48 && path[i] < 48 + 10)) path[i] = '_';
	if (path.find(".dae") >= path.size() || path.find(".dae") < 0) path = path + ".dae";
	while (appState->states.remeshing);
	appState->states.pauseUpdation = true;

	this->worker_th = std::thread([path, mesh, this]() -> void {
		std::ofstream out_file(path);
		if (!out_file.is_open())
		{
			Log("Failed to open file " + path);
			this->exportProgress = 0.0f;
			delete mesh;
			return;
		}

		std::stringstream pos_strm, txc_strm, ind_strm;


		float tmp = 0.0f;

		static char buffer[1024];

		for (int i = 0; i < mesh->vertexCount; i++)
		{
			const auto& vert = mesh->vert[i];
			auto tmp = 0.0f; appState->mainMap.currentTileDataLayers[0]->GetPixelF(vert.texCoord.x, vert.texCoord.y, &tmp);
			auto new_pos = vert.position + vert.normal * tmp;
			pos_strm << new_pos.x << ' ' << new_pos.y << ' ' << new_pos.z << ' ';
			txc_strm << vert.texCoord.x << ' ' << vert.texCoord.y << ' ';
		}

		appState->states.pauseUpdation = false;

		tmp = 1.0f / mesh->indexCount;
		for (int i = 0; i < mesh->indexCount; i++)
		{
			ind_strm << mesh->indices[i] << ' ' << mesh->indices[i] << ' ';
			this->exportProgress = ((float)i * tmp) * 0.2f + 0.7f;
		}

		std::string creation_timestamp = currentDateTime();

		std::stringstream xml_data;

		xml_data << R"(<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <asset>
    <contributor>
      <author>TerraForge3D User</author>
      <authoring_tool>TerraForge3D Gen 2</authoring_tool>
    </contributor>
    <created>)" << creation_timestamp << R"(</created>
    <modified>)" << creation_timestamp << R"(</modified>
    <unit name="meter" meter="1"/>
    <up_axis>Y_UP</up_axis>
  </asset>
  <library_images/>
  <library_geometries>
    <geometry id="MainOBJ-mesh" name="MainOBJ">
      <mesh>
        <source id="MainOBJ-mesh-positions">
          <float_array id="MainOBJ-mesh-positions-array" count=")" << (mesh->vertexCount * 3) << R"(">)" << pos_strm.str() << R"(</float_array>
          <technique_common>
            <accessor source="#MainOBJ-mesh-positions-array" count=")" << mesh->vertexCount << R"(" stride="3">
              <param name="X" type="float"/>
              <param name="Y" type="float"/>
              <param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <source id="MainOBJ-mesh-map-0">
          <float_array id="MainOBJ-mesh-map-0-array" count=")" << (mesh->vertexCount * 2) << R"(">)" << txc_strm.str() << R"(</float_array>
          <technique_common>
            <accessor source="#MainOBJ-mesh-map-0-array" count=")" << mesh->vertexCount << R"(" stride="2">
              <param name="S" type="float"/>
              <param name="T" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <vertices id="MainOBJ-mesh-vertices">
          <input semantic="POSITION" source="#MainOBJ-mesh-positions"/>
        </vertices>
        <triangles count=")" << (mesh->indexCount / 3) << R"(">
          <input semantic="VERTEX" source="#MainOBJ-mesh-vertices" offset="0"/>
          <input semantic="TEXCOORD" source="#MainOBJ-mesh-map-0" offset="1" set="0"/>
          <p>)" << ind_strm.str() << R"(</p>
        </triangles>
      </mesh>
    </geometry>
  </library_geometries>
  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">
      <node id="MainOBJ" name="MainOBJ" type="NODE">
        <matrix sid="transform">1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</matrix>
        <instance_geometry url="#MainOBJ-mesh" name="MainOBJ"/>
      </node>
    </visual_scene>
  </library_visual_scenes>
  <scene>
    <instance_visual_scene url="#Scene"/>
  </scene>
</COLLADA>
)";


		out_file << xml_data.str();

		out_file.flush();
		out_file.close();

		this->exportProgress = 1.1f;

		delete mesh;
		});
	this->worker_th.detach();
}

void ExportManager::ExportMeshGLTF(std::string path, Mesh* mesh)
{
	this->exportProgress = 0.01f;
	for (int i = path.size() - 3; i < path.size(); i++) if (!(path[i] >= 65 && path[i] < 65 + 26) && !(path[i] >= 97 && path[i] < 97 + 26) && !(path[i] >= 48 && path[i] < 48 + 10)) path[i] = '_';
	if (path.find(".gltf") >= path.size() || path.find(".gltf") < 0) path = path + ".gltf";
	while (appState->states.remeshing);
	appState->states.pauseUpdation = true;

	this->worker_th = std::thread([path, mesh, this]() -> void {
		std::ofstream out_file(path);
		if (!out_file.is_open())
		{
			Log("Failed to open file " + path);
			this->exportProgress = 0.0f;
			delete mesh;
			return;
		}

		const auto fs_path = std::filesystem::path(path);
		std::string bin_path = path + ".bin";
		std::string bin_uri = fs_path.filename().string() + ".bin";

		nlohmann::json gltf_js, tmp_js, tmp2_js, tmp3_js;

		tmp_js["generator"] = "TerraForge3D Gen 2";
		tmp_js["version"] = "2.0";
		gltf_js["asset"] = tmp_js; tmp_js = nlohmann::json();

		gltf_js["scene"] = 0;

		tmp2_js["name"] = "Scene";
		tmp2_js["nodes"] = {0};
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
		tmp_js["count"] = mesh->vertexCount;
		tmp_js["type"] = "VEC3";
		gltf_js["accessors"].push_back(tmp_js); tmp_js = nlohmann::json();

		tmp_js["bufferView"] = 1;
		tmp_js["componentType"] = 5126;
		tmp_js["count"] = mesh->vertexCount;
		tmp_js["type"] = "VEC3";
		gltf_js["accessors"].push_back(tmp_js); tmp_js = nlohmann::json();

		tmp_js["bufferView"] = 2;
		tmp_js["componentType"] = 5126;
		tmp_js["count"] = mesh->vertexCount;
		tmp_js["type"] = "VEC2";
		gltf_js["accessors"].push_back(tmp_js); tmp_js = nlohmann::json();

		tmp_js["bufferView"] = 3;
		tmp_js["componentType"] = 5125;
		tmp_js["count"] = mesh->indexCount;
		tmp_js["type"] = "SCALAR";
		gltf_js["accessors"].push_back(tmp_js); tmp_js = nlohmann::json();

		gltf_js["bufferViews"] = nlohmann::json();

		auto byte_offset = 0u;
		tmp_js["buffer"] = 0;
		tmp_js["byteLength"] = sizeof(float) * 3 * mesh->vertexCount;
		tmp_js["byteOffset"] = byte_offset;
		gltf_js["bufferViews"].push_back(tmp_js); tmp_js = nlohmann::json();
		byte_offset += sizeof(float) * 3 * mesh->vertexCount;

		tmp_js["buffer"] = 0;
		tmp_js["byteLength"] = sizeof(float) * 3 * mesh->vertexCount;
		tmp_js["byteOffset"] = byte_offset;
		gltf_js["bufferViews"].push_back(tmp_js); tmp_js = nlohmann::json();
		byte_offset += sizeof(float) * 3 * mesh->vertexCount;

		tmp_js["buffer"] = 0;
		tmp_js["byteLength"] = sizeof(float) * 2 * mesh->vertexCount;
		tmp_js["byteOffset"] = byte_offset;
		gltf_js["bufferViews"].push_back(tmp_js); tmp_js = nlohmann::json();
		byte_offset += sizeof(float) * 2 * mesh->vertexCount;

		tmp_js["buffer"] = 0;
		tmp_js["byteLength"] = sizeof(uint32_t) * mesh->indexCount;
		tmp_js["byteOffset"] = byte_offset;
		gltf_js["bufferViews"].push_back(tmp_js); tmp_js = nlohmann::json();
		byte_offset += sizeof(uint32_t) * mesh->indexCount;

		gltf_js["buffers"] = nlohmann::json();
		
		tmp_js["byteLength"] = byte_offset;
		tmp_js["uri"] = bin_uri;
		gltf_js["buffers"].push_back(tmp_js); tmp_js = nlohmann::json();

		out_file << gltf_js.dump(4);

		out_file.flush();
		out_file.close();

		// open c file
		FILE* c_file = fopen(bin_path.c_str(), "wb");
		if (!c_file)
		{
			Log("Failed to open file " + bin_path);
			this->exportProgress = 0.0f;
			delete mesh;
			return;
		}

		float tmp = 0.0f;

		// export gltf buffer according to the description above
		for (auto i = 0u; i < mesh->vertexCount; i++)
		{
			const auto& vert = mesh->vert[i];
			auto tmp = 0.0f; appState->mainMap.currentTileDataLayers[0]->GetPixelF(vert.texCoord.x, vert.texCoord.y, &tmp);
			auto new_pos = vert.position + vert.normal * tmp;
			float f_buffer[3] = { new_pos.x, new_pos.y, new_pos.z };
			fwrite(f_buffer, sizeof(float), 3, c_file);
		}

		this->exportProgress = 0.4f;

		appState->states.pauseUpdation = false;

		for (auto i = 0; i < mesh->vertexCount; i++)
		{
			auto& vert = mesh->vert[i];
			float f_buffer[3] = { vert.normal.x, vert.normal.y, vert.normal.z };
			fwrite(f_buffer, sizeof(float), 3, c_file);
		}

		this->exportProgress = 0.5f;


		for (auto i = 0; i < mesh->vertexCount; i++)
		{
			auto& vert = mesh->vert[i];
			float f_buffer[2] = { vert.texCoord.x, vert.texCoord.y };
			fwrite(f_buffer, sizeof(float), 2, c_file);
		}

		this->exportProgress = 0.6f;

		// write the indices as uint16_t
		for (auto i = 0u; i < mesh->indexCount; i++)
		{
			uint32_t index = (uint32_t)mesh->indices[i];
			fwrite(&index, sizeof(uint32_t), 1, c_file);
		}

		this->exportProgress = 0.9f;

		fflush(c_file);
		fclose(c_file);


		this->exportProgress = 1.1f;

		delete mesh;
		});
	this->worker_th.detach();
}