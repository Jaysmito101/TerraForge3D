#include "Exporters/ExportManager.h"
#include "Data/ApplicationState.h"
#include "Base/Base.h"


#include <sstream>
#include <fstream>

#include <iostream>
#include <string>
#include <stdio.h>
#include <filesystem>
#include <time.h>



Mesh* ExportManager::ApplyMeshTransform(Mesh* mesh, float* data)
{
	int size = m_AppState->mainMap.tileResolution, x = 0, y = 0;
	for (int i = 0; i < mesh->GetVertexCount(); i++)
	{
		const auto& vert = mesh->GetVertex(i);
		x = std::clamp((int)(vert.texCoord.x * size), 0, size - 1);
		y = std::clamp((int)(vert.texCoord.y * size), 0, size - 1);
		mesh->SetPosition(vert.position + vert.normal * data[y * size + x], i);
	}
	mesh->RecalculateNormals();
	return mesh;
}

bool ExportManager::ExportMesh(std::string path, Mesh* mesh, int format)
{
	auto fsPath = std::filesystem::path(path);
	auto filename0 = fsPath.filename().u8string();
	auto filename = std::string("");
	for (auto ch : filename0) if (std::isalnum(ch) || ch == ' ' || ch == '_') filename += ch;
	path = fsPath.parent_path().string() + "/" + filename;
	bool add_extension = fsPath.has_extension();
	switch (format)
	{
		case 0: path += ".obj"; return m_ObjExporter.Export(path, mesh, &m_ExportProgress);
		case 1: path += ".stl"; return m_StlExporter.ExportASCII(path, mesh, &m_ExportProgress);
		case 2: path += ".stl"; return m_StlExporter.ExportBinary(path, mesh, &m_ExportProgress);
		case 3: path += ".ply"; return m_PlyExporter.ExportASCII(path, mesh, &m_ExportProgress);
		case 4: path += ".ply"; return m_PlyExporter.ExportBinary(path, mesh, &m_ExportProgress);
		case 5: path += ".dae"; return m_DaeExporter.Export(path, mesh, &m_ExportProgress);
		case 6: path += ".gltf"; return m_GltfExporter.ExportGLTF(path, path + ".bin", mesh, &m_ExportProgress);
		default: return false;
	}
	return false;
}


void ExportManager::ExportMeshCurrentTile(std::string path, bool* exporting, int format, bool updateWorkerUpdation)
{
	if (exporting) *exporting = true;
	m_AppState->eventManager->RaiseEvent("ForceUpdate"); // force regenerate the mesh before exporting once
	auto heightMapData = m_AppState->generationManager->GetHeightmapData()->GetCPUCopy();
	auto meshClone = m_AppState->mainModel->mesh->Clone();
	auto worker = ([path, format, exporting, meshClone, heightMapData, this]()->void {
		using namespace std::chrono_literals;
		SetStatusMessage("Exporting : " + path);
		m_ExportProgress = 0.01f;
		if (!ExportMesh(path, ApplyMeshTransform(meshClone, heightMapData), format)) SetStatusMessage("Failed to export : " + path);
		else SetStatusMessage("");
		m_ExportProgress = 1.1f;
		delete meshClone;
		delete[] heightMapData;
		if (exporting) *exporting = false; 
		});
	m_AppState->jobSystem->AddFunctionWorker(worker);
}

void ExportManager::ExportMeshAllTiles(std::string pathStr, bool* exporting, int format)
{
	/*
	if (exporting) *exporting = true;
	hideExportControls = true;
	auto worker = std::thread([pathStr, format, exporting, this]()->void {
		using namespace std::chrono_literals;
		auto path = std::filesystem::path(pathStr);
		auto parentDir = path.parent_path().string();
		auto filename = path.filename().string();
		auto extension = std::string("");
		auto outFilename = std::string("");
		bool exportingTile = false;
		if (path.has_extension())
		{
			filename = filename.substr(0, filename.find_last_of("."));
			extension = path.extension().string();
		}
		auto previousTileX = appState->mainMap.currentTileX;
		auto previousTileY = appState->mainMap.currentTileY;
		SetStatusMessage("");
		//appState->workManager->SetUpdationPaused(true); // disable updation from main thread
		for(auto tx = 0; tx < appState->mainMap.tileCount; tx++)
		{
			for (auto ty = 0; ty < appState->mainMap.tileCount; ty++)
			{
				appState->mainMap.currentTileX = tx;
				appState->mainMap.currentTileY = ty;
				outFilename = parentDir + "/" + filename + "_" + std::to_string(tx) + "_" + std::to_string(ty) + extension;
				exportingTile = false; ExportMeshCurrentTile(outFilename, &exportingTile, format, false); // export tile mesh
				while (exportingTile) std::this_thread::sleep_for(100ms); // wait for the tile export to finish
			}
		}
		//appState->workManager->SetUpdationPaused(false); // enable updation from main thread
		SetStatusMessage("");
		appState->mainMap.currentTileX = previousTileX;
		appState->mainMap.currentTileY = previousTileY;
		if (exporting) *exporting = false;
		hideExportControls = false;
		});
	worker.detach();
	*/
}
