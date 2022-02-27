#pragma once

#include "glm/glm.hpp"
#include "json.hpp"

class LightManager
{
public:
	LightManager();
	~LightManager();

	void ShowSettings(bool renderWindow = false, bool* pOpen = nullptr);

	nlohmann::json Save();
	void Load(nlohmann::json data);

	float color[4];
	float ambientColor[4];
	float strength;
	glm::vec3 position;

	int lightManagerID;
};
