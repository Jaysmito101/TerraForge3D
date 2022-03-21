#include "Foliage/FoliagePlacement.h"
#include "Utils/Utils.h"
#include "Base/ModelImporter.h"
#include "Base/Shader.h"
#include "Base/Texture2D.h"
#include "Data/ProjectData.h"
#include "Data/ApplicationState.h"
#include "Platform.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <json/json.hpp>

static Texture2D *LoadTextureFromFile(Texture2D *texture)
{
	std::string path = ShowOpenFileDialog("*.png\0*.jpg\0");

	if(path.size() > 3)
	{
		if(texture)
		{
			delete texture;
		}

		return new Texture2D(path, false);
	}

	return texture;
}

FoliageItem::FoliageItem(std::string defaultTexture)
{
	albedo = new Texture2D(defaultTexture, false);
	ao = new Texture2D(defaultTexture, false);
	metallic = new Texture2D(defaultTexture, false);
	roughness = new Texture2D(defaultTexture, false);
	normal = new Texture2D(defaultTexture, false);
}

FoliageItem::~FoliageItem()
{
	if(albedo)
	{
		delete albedo;
	}

	if(ao)
	{
		delete ao;
	}

	if(metallic)
	{
		delete metallic;
	}

	if(roughness)
	{
		delete roughness;
	}

	if(normal)
	{
		delete normal;
	}

	if(model)
	{
		delete model;
	}
}


FoliageManager::FoliageManager(ApplicationState *as)
{
	appState = as;
}

FoliageManager::~FoliageManager()
{
	for(FoliageItem *item : foliageItems)
	{
		delete item;
	}

	foliageItems.clear();
}

void FoliageManager::ShowSettings(bool *pOpen)
{
	ImGui::Begin("Foliage Manager", pOpen);
	int id = 0;

	for(FoliageItem *item : foliageItems)
	{
		ImGui::PushID(id++);
		ImGui::Text("Name : %s", item->name.data());

		if(ImGui::ImageButton((ImTextureID)item->albedo->GetRendererID(), ImVec2(50, 50)))
		{
			item->albedo = LoadTextureFromFile(item->albedo);
		}

		ImGui::SameLine();

		if(ImGui::ImageButton((ImTextureID)item->normal->GetRendererID(), ImVec2(50, 50)))
		{
			item->normal = LoadTextureFromFile(item->normal);
		}

		ImGui::SameLine();

		if(ImGui::ImageButton((ImTextureID)item->ao->GetRendererID(), ImVec2(50, 50)))
		{
			item->ao = LoadTextureFromFile(item->ao);
		}

		if(ImGui::ImageButton((ImTextureID)item->metallic->GetRendererID(), ImVec2(50, 50)))
		{
			item->metallic = LoadTextureFromFile(item->metallic);
		}

		ImGui::SameLine();

		if(ImGui::ImageButton((ImTextureID)item->roughness->GetRendererID(), ImVec2(50, 50)))
		{
			item->roughness = LoadTextureFromFile(item->roughness);
		}

		ImGui::DragFloat3("Position##FItem", glm::value_ptr(item->model->position), 0.1f);
		ImGui::DragFloat3("Rotation##FItem", glm::value_ptr(item->model->rotation), 0.1f);
		ImGui::DragFloat3("Scale##FItem", glm::value_ptr(item->model->scale), 0.1f);
		ImGui::Separator();
		ImGui::PopID();
	}

	if(ImGui::Button("Add Item"))
	{
		std::string path = ShowOpenFileDialog(".obj\0.fbx\0.gltf\0");

		if(path.size() > 3)
		{
			foliageItems.push_back(new FoliageItem(appState->constants.texturesDir + PATH_SEPARATOR "white.png"));
			foliageItems.back()->name = path.substr(path.find(PATH_SEPARATOR) + 1);
			foliageItems.back()->model = LoadModel(path);
			foliageItems.back()->model->SetupMeshOnGPU();
			foliageItems.back()->model->UploadToGPU();
		}

		else
		{
			Log("Invalid Path : " + path);
		}
	}

	if(ImGui::Button("Reset Shaders"))
	{
		delete appState->shaders.foliage;
		bool res = false;
		appState->shaders.foliage = new Shader(ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "foliage" PATH_SEPARATOR "vert.glsl", &res),
		                                       ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "foliage" PATH_SEPARATOR "frag.glsl", &res),
		                                       ReadShaderSourceFile(appState->constants.shadersDir + PATH_SEPARATOR "foliage" PATH_SEPARATOR "geom.glsl", &res));
	}

	ImGui::End();
}

void FoliageManager::RenderFoliage(Camera &camera)
{
	Shader *shader = appState->shaders.foliage;

	for(FoliageItem *item : foliageItems)
	{
		item->albedo->Bind(1);
		shader->SetUniformi("_Albedo", 1);
		item->ao->Bind(2);
		shader->SetUniformi("_AO", 2);
		item->metallic->Bind(3);
		shader->SetUniformi("_Metallic", 3);
		item->roughness->Bind(4);
		shader->SetUniformi("_Roughness", 4);
		item->normal->Bind(5);
		shader->SetUniformi("_Normal", 5);
		item->model->Update();
		glm::mat4 temp = camera.pv * item->model->modelMatrix;
		shader->SetMPV(temp);
		shader->SetUniformMat4("_Model", item->model->modelMatrix);
		item->model->Render();
	}
}

nlohmann::json FoliageManager::Save()
{
	nlohmann::json data;
	Log("Foliage Manager does not support Save/Load yet!");
	return data;
}

void FoliageManager::Load(nlohmann::json data)
{
	Log("Foliage Manager does not support Save/Load yet!");
}
