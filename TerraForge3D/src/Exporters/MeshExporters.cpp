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
		for (int i = 0; i < mesh->vertexCount; i++)
		{
			const auto& vert = mesh->vert[i];
			auto tmp = 0.0f; appState->mainMap.currentTileDataLayers[0]->GetPixelF(vert.texCoord.x, vert.texCoord.y, &tmp);
			mesh->vert[i].position = vert.position + vert.normal * tmp;
		}
		appState->states.pauseUpdation = false;
		mesh->RecalculateNormals();
		daeExporter.Export(path, mesh, &this->exportProgress);

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
		const auto fs_path = std::filesystem::path(path);
		std::string bin_path = fs_path.filename().string() + ".bin";

		for (int i = 0; i < mesh->vertexCount; i++)
		{
			const auto& vert = mesh->vert[i];
			auto tmp = 0.0f; appState->mainMap.currentTileDataLayers[0]->GetPixelF(vert.texCoord.x, vert.texCoord.y, &tmp);
			mesh->vert[i].position = vert.position + vert.normal * tmp;
		}
		appState->states.pauseUpdation = false;
		mesh->RecalculateNormals();
		gltfExporter.ExportGLTF(path, bin_path, mesh, &this->exportProgress);		

		this->exportProgress = 1.1f;

		delete mesh;
		});
	this->worker_th.detach();
}