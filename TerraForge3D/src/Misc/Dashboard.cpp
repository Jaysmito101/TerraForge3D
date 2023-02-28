#include "Misc/Dashboard.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

Dashboard::Dashboard(ApplicationState* appState)
{
	m_AppState = appState;
}

Dashboard::~Dashboard()
{
}

void Dashboard::Update()
{
	if(m_ForceUpdate) m_AppState->workManager->SetRequireRemesh(true);
}

void Dashboard::ShowSettings()
{
	ImGui::Begin("Dashboard", &m_IsWindowVisible);
	ShowChooseBaseModelPopup();
	if (ImGui::Button("Change Base Model")) ImGui::OpenPopup("Choose Base Model##Main");

	if (ImGui::CollapsingHeader("Generator Resolution Settings"))
	{
		bool changed = false;
		changed = PowerOfTwoDropDown("Tile Resolution##MainMapGen", &m_AppState->mainMap.tileResolution, 4, 32) || changed;
		changed = ImGui::DragInt("Tile Count##MainMapGen", &m_AppState->mainMap.tileCount, 0.01f, 0, 1000000) || changed;

		if (changed)
		{
			m_AppState->workManager->WaitForFinish();
			m_AppState->mainMap.currentTileX = m_AppState->mainMap.currentTileY = 0;
			CalculateTileSizeAndOffset();
			for (int i = 0; i < 6; i++) m_AppState->mainMap.currentTileDataLayers[i]->Resize(m_AppState->mainMap.tileResolution);
			m_AppState->workManager->Resize();
		}
		ImGui::DragInt("Current Tile X##MainMapGen", &m_AppState->mainMap.currentTileX, 0.01f, 0, m_AppState->mainMap.tileCount);
		ImGui::DragInt("Current Tile Y##MainMapGen", &m_AppState->mainMap.currentTileY, 0.01f, 0, m_AppState->mainMap.tileCount);
		ImGui::Text("Map Resolution: %d", m_AppState->mainMap.mapResolution);
		ImGui::Text("Tile Size: %f", m_AppState->mainMap.tileSize);
		ImGui::Text("Tile Offset: %f %f", m_AppState->mainMap.tileOffsetX, m_AppState->mainMap.tileOffsetY);
		CalculateTileSizeAndOffset();
	}

	ImGui::NewLine();
	ImGui::Checkbox("Force Update", &m_ForceUpdate);
	ImGui::Checkbox("Auto Save", &m_AppState->states.autoSave);
	ImGui::NewLine();

	if (ImGui::Button("Regenerate")) m_AppState->workManager->SetRequireRemesh(true);

	ImGui::Separator();
	ImGui::NewLine();
	ImGui::Separator();
	ImGui::End();
}

void Dashboard::ShowChooseBaseModelPopup()
{
	if (ImGui::BeginPopup("Choose Base Model##Main"))
	{
		if (ImGui::Button("Open From File"))
		{
			std::string path = ShowOpenFileDialog("*.*");
			if (FileExists(path))
			{
				Model* tmp = LoadModel(path);
				if (tmp) { delete m_AppState->mainModel; m_AppState->mainModel = tmp; tmp->mesh->RecalculateNormals(); tmp->SetupMeshOnGPU(); tmp->UploadToGPU(); }
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
		
		static const char* default_base_models[] = { "Plane", "Sphere", "Torus" };
		static int selected_item = 0;
		if (ImGui::BeginCombo("Gnerate Base Model##Choose Base Model Main", default_base_models[selected_item]))
		{
			for (int i = 0; i < IM_ARRAYSIZE(default_base_models); i++)
			{
				bool is_selected = (selected_item == i);
				if (ImGui::Selectable(default_base_models[i], is_selected)) selected_item = i;
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (selected_item == 0)
		{
			// plane
			static int resolution = 256;
			static float scale = 1.0f;
			ImGui::DragFloat("Scale##GenBasePlane", &scale, 0.01f, 0.05f, 10000.0f);
			ImGui::DragInt("Resolution##GenBasePlane", &resolution, 0.1f, 2, 100000);
			resolution = std::clamp(resolution, 2, 1024 * 128);
			if (ImGui::Button("Generate Plane##GenBasePlane"))
			{
				m_AppState->mainModel->mesh->GeneratePlane(resolution, scale);
				m_AppState->mainModel->mesh->RecalculateNormals();
				m_AppState->mainModel->SetupMeshOnGPU();
				m_AppState->mainModel->UploadToGPU();
			}
		}
		else if (selected_item == 1)
		{
			// sphere
			static int resolution = 256;
			static float scale = 1.0f;
			ImGui::DragFloat("Radius##GenBaseSphere", &scale, 0.01f, 0.05f, 10000.0f);
			ImGui::DragInt("Resolutione##GenBaseSphere", &resolution, 0.1f, 1, 100000);
			resolution = std::clamp(resolution, 1, 100000);
			if (ImGui::Button("Generate Sphere##GenBaseSphere"))
			{
				m_AppState->mainModel->mesh->GenerateSphere(resolution, scale);
				m_AppState->mainModel->mesh->RecalculateNormals();
				m_AppState->mainModel->SetupMeshOnGPU();
				m_AppState->mainModel->UploadToGPU();
			}
		}
		else
		{
			static float outerRadius = 1.0f;
			static float innerRadius = 0.5f;
			static int numSegments = 256;
			static int numRings = 32;
			ImGui::DragFloat("Outer Radius##GenBaseTorus", &outerRadius, 0.01f, 0.05f, 10000.0f);
			ImGui::DragFloat("Inner Radius##GenBaseTorus", &innerRadius, 0.01f, 0.05f, 10000.0f);
			if(ImGui::DragInt("Segments##GenBaseTorus", &numSegments, 0.1f, 1, 100000)) numSegments = std::clamp(numSegments, 1, 100000);
			if(ImGui::DragInt("Rings##GenBaseTorus", &numRings, 0.1f, 1, 100000)) numRings = std::clamp(numRings, 1, 100000);
			if (ImGui::Button("Generate Torus##GenBaseTorus"))
			{
				m_AppState->mainModel->mesh->GenerateTorus(outerRadius, innerRadius, numSegments, numRings);
				m_AppState->mainModel->mesh->RecalculateNormals();
				m_AppState->mainModel->SetupMeshOnGPU();
				m_AppState->mainModel->UploadToGPU();
			}
		}
		ImGui::EndPopup();
	}
}

void Dashboard::CalculateTileSizeAndOffset()
{
	m_AppState->mainMap.tileCount = std::clamp(m_AppState->mainMap.tileCount, 1, 1000000);
	m_AppState->mainMap.mapResolution = m_AppState->mainMap.tileResolution * m_AppState->mainMap.tileCount;
	m_AppState->mainMap.tileSize = (1.0f / m_AppState->mainMap.tileCount);
	m_AppState->mainMap.currentTileX = std::clamp(m_AppState->mainMap.currentTileX, 0, m_AppState->mainMap.tileCount - 1);
	m_AppState->mainMap.currentTileY = std::clamp(m_AppState->mainMap.currentTileY, 0, m_AppState->mainMap.tileCount - 1);
	m_AppState->mainMap.tileOffsetX = m_AppState->mainMap.currentTileX * m_AppState->mainMap.tileSize;
	m_AppState->mainMap.tileOffsetY = m_AppState->mainMap.currentTileY * m_AppState->mainMap.tileSize;
}

