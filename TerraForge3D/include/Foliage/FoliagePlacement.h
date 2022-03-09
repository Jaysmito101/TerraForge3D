#pragma once

#include <string>
#include <vector>
#include "Base/Model.h"
#include "Base/Texture2D.h"
#include "Base/Camera.h"
#include "Base/Shader.h"

struct FoliageItem
{
	std::string name;
	std::string modelDir;
	Texture2D *texture;
	bool textureLoaded = false;
	Model *model;
	bool active = true;
};

void SetupFoliageManager();

void ShowFoliageManager(bool *pOpen);

void RenderFoliage(Shader *shader, Camera &camera);