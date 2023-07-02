#include "Exporters/ExportManager.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

#include <sstream>
#include <fstream>

ExportManager::ExportManager(ApplicationState* as)
{
	this->appState = as;
}

ExportManager::~ExportManager()
{
}

void ExportManager::ShowSettings()
{
	if (!this->isWindowOpen) return;
	ImGui::Begin("Export Manager##RootWindow", &this->isWindowOpen);

	if (this->exportProgress > 0.0f || hideExportControls)
	{
		if (this->statusMessage.size() > 0) ImGui::Text(this->statusMessage.data());
		ImGui::ProgressBar(this->exportProgress);
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
	if(this->exportProgress > 1.0f ) this->exportProgress = 0.0f;
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

	if (ImGui::BeginCombo("Export Format", exportTypes[this->exportMeshFormat]))
	{
		for (int i = 0; i < IM_ARRAYSIZE(exportTypes); i++)
		{
			bool is_selected = (this->exportMeshFormat == i);
			if (ImGui::Selectable(exportTypes[i], is_selected)) this->exportMeshFormat = i;
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("Export Current Tile"))
	{
		std::string output_file_path = ShowSaveFileDialog("*.*");
		if (output_file_path.size() < 3) return;
		this->ExportMeshCurrentTile(output_file_path, nullptr, this->exportMeshFormat);
	}

	ImGui::BeginDisabled();
	if (ImGui::Button("Export All Tiles"))
	{
		std::string output_file_path = ShowSaveFileDialog("*.*");
		if (output_file_path.size() < 3) return;
		this->ExportMeshAllTiles(output_file_path, nullptr, this->exportMeshFormat);
	}
	ImGui::EndDisabled();
}

void ExportManager::ShowTextureExportSettings()
{
	static const char* exportTypes[] = {
		"PNG",
		"JPG",
		"BMP",
		"TGA"
	};

	static const char* exportBitDepths[] = {
		"8-bit",
		"16-bit",
		"32-bit"
	};

	if (ImGui::BeginCombo("Export Format", exportTypes[this->exportTextureFormat]))
	{
		for (int i = 0; i < IM_ARRAYSIZE(exportTypes); i++)
		{
			bool is_selected = (this->exportTextureFormat == i);
			if (ImGui::Selectable(exportTypes[i], is_selected)) this->exportTextureFormat = i;
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (ImGui::BeginCombo("Bit Depth", exportBitDepths[this->exportTextureBitDepth]))
	{
		for (int i = 0; i < IM_ARRAYSIZE(exportBitDepths); i++)
		{
			bool is_selected = (this->exportTextureBitDepth == i);
			if (ImGui::Selectable(exportBitDepths[i], is_selected)) this->exportTextureBitDepth = i;
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("Export Current Tile"))
	{
		std::string output_file_path = ShowSaveFileDialog("*.*");
		if (output_file_path.size() < 3) return;
		this->ExportTextureCurrentTile(output_file_path, this->exportTextureFormat, this->exportTextureBitDepth, nullptr);
	}

	ImGui::BeginDisabled();
	if (ImGui::Button("Export All Tiles"))
	{
		std::string output_file_path = ShowSaveFileDialog("*.*");
		if (output_file_path.size() < 3) return;
		// this->ExportTextureAllTiles(output_file_path, nullptr, this->exportTextureFormat, this->exportTextureBitDepth);
	}
	ImGui::EndDisabled();
}
