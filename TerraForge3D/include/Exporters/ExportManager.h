#pragma once

#include <thread>
#include <string>

#include "Base/Base.h"

#include "Generators/GeneratorTexture.h"

#include "Exporters/OBJExporter.h"
#include "Exporters/STLExporter.h"
#include "Exporters/PLYExporter.h"
#include "Exporters/GLTFExporter.h"
#include "Exporters/ColladaExporter.h"

#include "Exporters/RawTextureExporter.h"
#include "Exporters/ExrTextureExporter.h"
#include "Exporters/WebpTextureExporter.h"
#include "Exporters/PngTextureExporter.h"


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
	void ExportTextureCurrentTile(std::string path, int format, int bitDepth, bool* exporting);

public:
	void ShowMeshExportSettings();
	void ShowTextureExportSettings();
	Mesh* ApplyMeshTransform(Mesh* mesh, float* data);
	bool ExportMesh(std::string path, Mesh* mesh, int format);

	void UpdateHeightmapVisualizer();
	float* ApplyHeightmapTextureTransform(float* data, float* minMax);
	bool ExportHeightmapTexture(std::string path, float* data, int format, int bitDepth, int resolution);
	
	
	inline void SetStatusMessage(std::string msg) { this->m_StatusMessage = msg; }
	inline bool IsWindowOpen() { return this->m_IsWindowOpen; }
	inline bool* IsWindowOpenPtr() { return &this->m_IsWindowOpen; }
	inline void SetVisible(bool visible) { this->m_IsWindowOpen = visible; }

public:
	float m_ExportProgress = 0.0f;

private:
	ApplicationState* m_AppState = nullptr;
	std::string m_StatusMessage = "";
	int m_ExportMeshFormat = 0;
	int m_ExportTextureFormat = 0;
	int m_ExportTextureBitDepth = 0;
	float m_ExportHeightmapMinMaxHeight[2] = { -1.0f, 1.0f };
	bool m_HideExportControls = false;
	bool m_IsWindowOpen = false;

	std::shared_ptr<GeneratorTexture> m_VisualzeTexture;
	std::shared_ptr<ComputeShader> m_VisualzeShader;

	OBJExporter m_ObjExporter;
	STLExporter m_StlExporter;
	PLYExporter m_PlyExporter;
	GLTFExporter m_GltfExporter;
	ColladaExporter m_DaeExporter;

	RawTextureExporter m_RawTextureExporter;
	ExrTextureExporter m_ExrTextureExporter;
	WebpTextureExporter m_WebpTextureExporter;
	PngTextureExporter m_PngTextureExporter;
};