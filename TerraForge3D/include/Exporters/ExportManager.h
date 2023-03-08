#pragma once

#include <thread>
#include <string>

#include "Exporters/OBJExporter.h"
#include "Exporters/STLExporter.h"
#include "Exporters/PLYExporter.h"
#include "Exporters/GLTFExporter.h"
#include "Exporters/ColladaExporter.h"

class ApplicationState; 
class Model;
class Mesh;

class ExportManager
{
public:
	ExportManager(ApplicationState* as);
	~ExportManager();

	void ShowSettings();

	void Update();
	
	void ExportMeshCurrentTile(std::string path, bool* exporting, int format, bool updateWorkerUpdation = true);
	void ExportMeshAllTiles(std::string path, bool* exporting, int format);

public:
	void ShowMeshExportSettings();
	void ShowTextureExportSettings();
	Mesh* ApplyMeshTransform(Mesh* mesh, float* data);
	bool ExportMesh(std::string path, Mesh* mesh, int format);

	inline void SetStatusMessage(std::string msg) { this->statusMessage = msg; }
	inline bool IsWindowOpen() { return this->isWindowOpen; }
	inline bool* IsWindowOpenPtr() { return &this->isWindowOpen; }
	inline void SetVisible(bool visible) { this->isWindowOpen = visible; }

public:
	float exportProgress = 0.0f;

private:
	ApplicationState* appState = nullptr;
	std::thread worker_th;
	std::string statusMessage = "";
	int exportMeshFormat = 0; 
	bool hideExportControls = false;
	bool isWindowOpen = false;
	OBJExporter objExporter;
	STLExporter stlExporter;
	PLYExporter plyExporter;
	GLTFExporter gltfExporter;
	ColladaExporter daeExporter;
};