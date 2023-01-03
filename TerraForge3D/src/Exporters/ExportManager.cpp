#include "Exporters/ExportManager.h"
#include "Data/ApplicationState.h"
#include "Base/Base.h"

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
	ImGui::Begin("Export Manager##RootWindow", &appState->windows.exportManager);

	if (this->exportProgress > 0.0f)
	{
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

	if (ImGui::Button("Export"))
	{
		std::string output_file_path = ShowSaveFileDialog("*.*");
		if (output_file_path.size() < 3) return;
		if (this->exportMeshFormat == 0) ExportMeshOBJ(output_file_path, appState->models.mainModel->mesh->Clone());
		if (this->exportMeshFormat == 1) ExportMeshSTLASCII(output_file_path, appState->models.mainModel->mesh->Clone());
		if (this->exportMeshFormat == 2) ExportMeshSTLBinary(output_file_path, appState->models.mainModel->mesh->Clone());
		if (this->exportMeshFormat == 3) ExportMeshPLYASCII(output_file_path, appState->models.mainModel->mesh->Clone());
		if (this->exportMeshFormat == 4) ExportMeshPLYBinary(output_file_path, appState->models.mainModel->mesh->Clone());
		if (this->exportMeshFormat == 5) ExportMeshCollada(output_file_path, appState->models.mainModel->mesh->Clone());
		if (this->exportMeshFormat == 6) ExportMeshGLTF(output_file_path, appState->models.mainModel->mesh->Clone());
	}
}

void ExportManager::ShowTextureExportSettings()
{
	ImGui::Text("TODO");
}

