#include "DrawFilter.h"
#include "Data/ApplicationState.h"
#include "imgui.h"

#include "Base/ImGuiShapes.h"

#include <iostream>

void DrawFilter::Render()
{
	Model *model;

	if(appState->mode == ApplicationMode::TERRAIN)
	{
		model = appState->models.coreTerrain;
	}

	else if(appState->mode == ApplicationMode::CUSTOM_BASE)
	{
		model = appState->models.customBase;
	}

	else
	{
		return;
	}

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1, 1, 1, 1));
	ImGui::BeginChild("##DrawFilterDrawArea", ImVec2(200, 200));

	if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		ImVec2 bmin = ImGui::GetItemRectMin();
		ImDrawList *list = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetMousePos();
		list->AddCircleFilled(pos, 10, ImColor(0, 0, 0));
		int res = model->mesh->res;
		pos.x -= bmin.x;
		pos.y -= bmin.y;
		pos.x *= res /200.0;
		pos.y *= res/ 200.0;

		for (int i = pos.y - radius; i < pos.y + radius; i++)
		{
			for (int j = pos.x - radius; j < pos.x + radius; j++)
			{
				model->mesh->AddElevation(strength, i, j);
			}
		}

		model->mesh->RecalculateNormals();
		model->UploadToGPU();
	}

	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::DragFloat("Brush Strength", &strength, 0.01f);
	ImGui::DragFloat("Brush Radius", &radius, 0.01f);
}

nlohmann::json DrawFilter::Save()
{
	return nlohmann::json();
}

void DrawFilter::Load(nlohmann::json data)
{
}

void DrawFilter::Apply()
{
}
