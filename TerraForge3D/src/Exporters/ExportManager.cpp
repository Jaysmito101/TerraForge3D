#include "Exporters/ExportManager.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

#include <sstream>
#include <fstream>

ExportManager::ExportManager(ApplicationState* as)
{
	m_AppState = as;
	m_VisualzeTexture = std::make_shared<GeneratorTexture>(256, 256);
	const auto shaderSource = ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "exporters" PATH_SEPARATOR "texture_export_visualizer.glsl", &s_TempBool);
	m_VisualzeShader = std::make_shared<ComputeShader>(shaderSource);
}

ExportManager::~ExportManager()
{
}

void ExportManager::ShowSettings()
{
	if (!m_IsWindowOpen) return;
	ImGui::Begin("Export Manager##RootWindow", &m_IsWindowOpen);

	if (m_ExportProgress > 0.0f || m_HideExportControls)
	{
		if (m_StatusMessage.size() > 0) ImGui::Text(m_StatusMessage.data());
		ImGui::ProgressBar(m_ExportProgress);
	}
	else
	{
		if (ImGui::BeginTabBar("Export Type"))
		{
			if (ImGui::BeginTabItem("Mesh"))
			{
				ShowMeshExportSettings();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Texture"))
			{
				ShowTextureExportSettings();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}

	ImGui::End();
}

void ExportManager::Update()
{
	if(m_ExportProgress > 1.0f ) m_ExportProgress = 0.0f;
}

void ExportManager::ShowMeshExportSettings()
{
	static const char* exportTypes[] = {
		"WaveFont OBJ",
		"STL ASCII",
		"STL Binary",
		"PLY ASCII",
		"PLY Binary",
		"Collada",
		"GLTF"
	};

	if (ImGui::BeginCombo("Export Format", exportTypes[m_ExportMeshFormat]))
	{
		for (int i = 0; i < IM_ARRAYSIZE(exportTypes); i++)
		{
			bool is_selected = (m_ExportMeshFormat == i);
			if (ImGui::Selectable(exportTypes[i], is_selected)) m_ExportMeshFormat = i;
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("Export Current Tile"))
	{
		std::string output_file_path = ShowSaveFileDialog("*.*");
		if (output_file_path.size() < 3) return;
		ExportMeshCurrentTile(output_file_path, nullptr, m_ExportMeshFormat);
	}

	ImGui::BeginDisabled();
	if (ImGui::Button("Export All Tiles"))
	{
		std::string output_file_path = ShowSaveFileDialog("*.*");
		if (output_file_path.size() < 3) return;
		ExportMeshAllTiles(output_file_path, nullptr, m_ExportMeshFormat);
	}
	ImGui::EndDisabled();
}

void ExportManager::ShowTextureExportSettings()
{
	static const char* exportTypes[] = {
		"PNG",
		"WEBP",
		"RAW",
		"EXR"
	};

	static const char* exportBitDepths[] = {
		"8-bit",
		"16-bit",
		"32-bit"
	};

	if (ImGui::BeginCombo("Export Format", exportTypes[m_ExportTextureFormat]))
	{
		for (int i = 0; i < IM_ARRAYSIZE(exportTypes); i++)
		{
			bool is_selected = (m_ExportTextureFormat == i);
			if (ImGui::Selectable(exportTypes[i], is_selected)) m_ExportTextureFormat = i;
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (ImGui::BeginCombo("Bit Depth", exportBitDepths[m_ExportTextureBitDepth]))
	{
		for (int i = 0; i < IM_ARRAYSIZE(exportBitDepths); i++)
		{
			bool is_selected = (m_ExportTextureBitDepth == i);
			if (ImGui::Selectable(exportBitDepths[i], is_selected)) m_ExportTextureBitDepth = i;
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::DragFloat2("Heightmap Min Max", m_ExportHeightmapMinMaxHeight, 0.01f);

	if (ImGui::Button("Auto Calculate Min Max"))
	{
		auto heightmapData = m_AppState->generationManager->GetHeightmapData()->GetCPUCopy();
		auto resolution = m_AppState->mainMap.mapResolution;
		auto [minHeight, maxHeight] = std::minmax_element(heightmapData, heightmapData + resolution * resolution);
		m_ExportHeightmapMinMaxHeight[0] = *minHeight; m_ExportHeightmapMinMaxHeight[1] = *maxHeight;
		delete[] heightmapData;
	}
	ImGui::Text("This option sometimes gives bad results with high resolution maps.");


	UpdateHeightmapVisualizer();
	ImGui::Image(m_VisualzeTexture->GetTextureID(), ImVec2(350, 350));

	if (ImGui::Button("Export Current Tile"))
	{
		std::string output_file_path = ShowSaveFileDialog("*.*");
		if (output_file_path.size() < 3) return;
		ExportTextureCurrentTile(output_file_path, m_ExportTextureFormat, (int)pow(2, m_ExportTextureBitDepth) * 8, nullptr);
	}

	ImGui::BeginDisabled();
	if (ImGui::Button("Export All Tiles"))
	{
		std::string output_file_path = ShowSaveFileDialog("*.*");
		if (output_file_path.size() < 3) return;
		// ExportTextureAllTiles(output_file_path, nullptr, exportTextureFormat, exportTextureBitDepth);
	}
	ImGui::EndDisabled();
}
