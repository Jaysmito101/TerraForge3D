#pragma once

#include <string>
#include <vector>

#include <json/json.hpp>

#include "Base/Model.h"
#include "Base/Texture2D.h"
#include "Base/Camera.h"
#include "Base/Shader.h"

class ApplicationState;

struct FoliageItem
{
	std::string name = "";
	std::string modelDir = "";
	Texture2D *roughness = nullptr;
	Texture2D *albedo = nullptr;
	Texture2D *ao = nullptr;
	Texture2D *normal = nullptr;
	Texture2D *metallic = nullptr;
	Model *model = nullptr;
	bool active = true;

	FoliageItem(std::string defaultTexture);
	~FoliageItem();
};

class FoliageManager
{
public:
	FoliageManager(ApplicationState *appState);
	~FoliageManager();

	void ShowSettings(bool *pOpen);

	void RenderFoliage(Camera &camera);

	nlohmann::json Save();

	void Load(nlohmann::json data);
public:
	ApplicationState *appState;
	std::vector<FoliageItem *> foliageItems;
};