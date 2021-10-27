#pragma once

#include "Filter.h"

class ErosionFilter : public Filter {

public:
	ErosionFilter(Model* model) 
	:Filter(model, "Simple Erosion Filter (CPU)"){}

	virtual void Render() override;
	virtual nlohmann::json Save() override;
	virtual void Load(nlohmann::json data) override;
	virtual void Apply() override;

	void trace(int x, int y);

	float iterationScale = 0.1f;
	float erosionRate = 0.01f;
	float friction = 0.1f;
	float speed = 0.1f;
	float depositionRate = 0.001f;
	int iterations = 24;
	int numParticles = 5000;
	float radius = 3;
};
