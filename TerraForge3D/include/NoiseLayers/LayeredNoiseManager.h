#pragma once

#include "NoiseLayer.h"
#include "json/json.hpp"

#include <vector>
#include <mutex>

class LayeredNoiseManager
{
public:
	LayeredNoiseManager();

	void UpdateLayers();
	bool Render();
	void Load(nlohmann::json data);
	nlohmann::json Save();
	float Evaluate(float x, float y, float z) const;

	float offset[3];
	float strength;
	bool absv; // Temporary
	bool sq;  // Temporary
	std::vector<NoiseLayer *> noiseLayers, toAdd;
	std::vector<int> toDelete;

	std::mutex mutex;
};
