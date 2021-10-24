#pragma once

#include "Filter.h"

class GPUErosionFilter : public Filter {

public:
	GPUErosionFilter(Model* model)
		:Filter(model, "Simple Erosion Filter (GPU)") {}

	virtual void Render() override;
	virtual nlohmann::json Save() override;
	virtual void Load(nlohmann::json data) override;
	virtual void Apply() override;
	virtual void OnAttach() override;

	float dimension = 1000.0f;
	float tfac = 1000.0f;
	int iterations = 1024;
};

