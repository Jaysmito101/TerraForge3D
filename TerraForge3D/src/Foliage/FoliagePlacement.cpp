#include "Foliage/FoliagePlacement.h"
#include "Utils/Utils.h"
#include "Base/ModelImporter.h"
#include "Base/Shader.h"
#include "Data/ProjectData.h"
#include "Data/ApplicationState.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <json/json.hpp>


FoliageManager::FoliageManager(ApplicationState *as)
{
	appState = as;
}

FoliageManager::~FoliageManager()
{
	for(FoliageItem item : foliageItems)
	{
		delete item.albedo;
		delete item.ao;
		delete item.specular;
		delete item.roughness;
		delete item.normal;
		delete item.model;
	}

	foliageItems.clear();
}

void FoliageManager::ShowSettings(bool *pOpen)
{
	ImGui::Begin("Foliage Manager", pOpen);
	ImGui::Text("WORK IN PROGRESS!");
	ImGui::End();
}

void FoliageManager::RenderFoliage(Camera &camera)
{
}

nlohmann::json FoliageManager::Save()
{
	nlohmann::json data;
	return data;
}

void FoliageManager::Load(nlohmann::json data)
{
}