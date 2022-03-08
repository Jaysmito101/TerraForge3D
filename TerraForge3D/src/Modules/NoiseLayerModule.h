#pragma once

#include "Terr3DModule.h"
#include "json.hpp"

class NoiseLayerModule : public Module
{
public:
	NoiseLayerModule(std::string id, ModuleManager *manager);

	void Render();
	nlohmann::json Save();
	void Load(nlohmann::json data);
	float Evaluate(float x, float y, float z);

	std::string name;
	bool active;
};