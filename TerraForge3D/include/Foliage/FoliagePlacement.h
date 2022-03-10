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
	std::string name;
	std::string modelDir;
	Texture2D *roughness;
	Texture2D *specular;
	Texture2D *albedo;
	Texture2D *ao;
	Texture2D *normal;
	Model *model;
	bool active = true;
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
	std::vector<FoliageItem> foliageItems;
};