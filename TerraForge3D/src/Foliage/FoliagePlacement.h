#pragma once

#include <string>
#include <vector>
#include <Model.h>
#include <Texture2D.h>
#include <Camera.h>
#include <Shader.h>

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