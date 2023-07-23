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

void ExportManager::UpdateHeightmapVisualizer()
{
	m_AppState->generationManager->GetHeightmapData()->Bind(0);	
	m_VisualzeTexture->BindForCompute(1);
	m_VisualzeShader->Bind();
	m_VisualzeShader->SetUniform2f("u_HmapMinMax", m_ExportHeightmapMinMaxHeight[0], m_ExportHeightmapMinMaxHeight[1]);
	m_VisualzeShader->SetUniform1i("u_Resolution", m_AppState->mainMap.tileResolution);
	const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	m_VisualzeShader->Dispatch(m_AppState->mainMap.tileResolution / workgroupSize, m_AppState->mainMap.tileResolution / workgroupSize, 1);
}

float* ExportManager::ApplyHeightmapTextureTransform(float* data, float* minMax)
{
	auto resolution = m_AppState->mainMap.mapResolution;
	auto minHeight = minMax, maxHeight = minMax + 1;
	
	// transform the heightmap data
	for (int i = 0 ; i < resolution * resolution ; i++)
		data[i] = (data[i] - *minHeight) / (*maxHeight - *minHeight);
	
	// flip the heightmap data
	for (int i = 0 ; i < resolution / 2 ; i++)
		for (int j = 0 ; j < resolution ; j++)
			std::swap(data[i * resolution + j], data[(resolution - i - 1) * resolution + j]);	
	return data;
}

bool ExportManager::ExportHeightmapTexture(std::string path, float* data, int format, int bitDepth, int resolution)
{
	auto fsPath = std::filesystem::path(path);
	auto filename0 = fsPath.filename().u8string();
	auto filename = std::string("");
	for (auto ch : filename0) if (std::isalnum(ch) || ch == ' ' || ch == '_') filename += ch;
	path = fsPath.parent_path().string() + "/" + filename;
	bool add_extension = fsPath.has_extension();
	switch (format)
	{
	case 0: path += ".png"; return m_PngTextureExporter.ExportHeightmap(path, data, bitDepth, resolution, &m_ExportProgress);
	case 1: path += ".webp"; return m_WebpTextureExporter.ExportHeightmap(path, data, bitDepth, resolution, &m_ExportProgress);
	case 2: path += ".raw"; return m_RawTextureExporter.ExportHeightmap(path, data, bitDepth, resolution, &m_ExportProgress);
	case 3: path += ".exr"; return m_ExrTextureExporter.ExportHeightmap(path, data, bitDepth, resolution, &m_ExportProgress);
	default: return false;
	}
	return false;
}


void ExportManager::ExportTextureCurrentTile(std::string path, int format, int bitDepth, bool* exporting)
{
	if (exporting) *exporting = true;
	m_AppState->eventManager->RaiseEvent("ForceUpdate"); // force regenerate the mesh before exporting once
	auto heightMapData = m_AppState->generationManager->GetHeightmapData()->GetCPUCopy();
	auto worker = ([path, format, exporting, bitDepth, heightMapData, this]()->void {
		using namespace std::chrono_literals;

		this->SetStatusMessage("Exporting : " + path);
		m_ExportProgress = 0.01f;
		
		if (!ExportHeightmapTexture(path, ApplyHeightmapTextureTransform(heightMapData, m_ExportHeightmapMinMaxHeight), format, bitDepth, m_AppState->mainMap.mapResolution)) this->SetStatusMessage("Failed to export : " + path);
		else this->SetStatusMessage("");
		
		m_ExportProgress = 1.1f;
		delete[] heightMapData;
		if (exporting) *exporting = false;
		});
	m_AppState->jobSystem->AddFunctionWorker(worker);
}
