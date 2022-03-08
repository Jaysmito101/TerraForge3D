#pragma once

#include "Filter.h"

class GPUErosionFilter : public Filter
{

public:
	GPUErosionFilter(Model *model)
		:Filter(model, "Simple Erosion Filter (GPU)") {}

	virtual void Render() override;
	virtual nlohmann::json Save() override;
	virtual void Load(nlohmann::json data) override;
	virtual void Apply() override;
	virtual void OnAttach() override;

	int mapSize;
	int numErosionIterations = 50000;
	int erosionBrushRadius = 3;
	int maxLifetime = 30;
	float sedimentCapacityFactor = 3;
	float minSedimentCapacity = 0.01f;
	float depositSpeed = 0.3f;
	float errodeSpeed = 0.3f;
	float evaporationSpeed = 0.01f;
	float gravity = 4;
	float startSpeed = 1;
	float startWater = 1;
	// (0 TO 1)
	float inertia = 0.3f;
};

