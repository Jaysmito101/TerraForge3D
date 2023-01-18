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


private:
	void ShowMeshExportSettings();
	void ShowTextureExportSettings();
	void ExportMeshOBJ(std::string path, Mesh* mesh);
	void ExportMeshSTLASCII(std::string path, Mesh* mesh);
	void ExportMeshSTLBinary(std::string path, Mesh* mesh);
	void ExportMeshPLYASCII(std::string path, Mesh* mesh);
	void ExportMeshPLYBinary(std::string path, Mesh* mesh);
	void ExportMeshCollada(std::string path, Mesh* mesh);
	void ExportMeshGLTF(std::string path, Mesh* mesh);

public:
	float exportProgress = 0.0f;

private:
	ApplicationState* appState = nullptr;
	std::thread worker_th;
	int exportMeshFormat = 0; 
	OBJExporter objExporter;
	STLExporter stlExporter;
	PLYExporter plyExporter;
	GLTFExporter gltfExporter;
	ColladaExporter daeExporter;
};