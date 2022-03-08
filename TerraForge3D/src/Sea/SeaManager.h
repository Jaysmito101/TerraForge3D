#pragma once

#include "json.hpp"

#include "Base/Base.h"

#include "Lighting/LightManager.h"

class SeaManager
{
public:

	SeaManager();
	~SeaManager();

	void Load(nlohmann::json data);
	nlohmann::json Save();

	void Render(Camera &camera, LightManager *lights, void *reflectionTexture, float time = 0);
	void ShowSettings(bool *pOpen);

	float level = 0.0f;
	bool enabled = true;

private:
	Model *model;
	Texture2D *dudvMap;
	Texture2D *normalMap;
	Shader *shader;

	float alpha = 0.5f;
	float distrotionScale = 1.0f;
	float distrotionStrength = 0.02f;
	float reflectivity = 0.7f;
	float waveSpeed = 0.01f;
	float scale = 1.0f;

	float color[3];

};
