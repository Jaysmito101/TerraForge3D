#pragma once

#include "Filter.h"

class ErosionFilter : public Filter {

public:
	ErosionFilter(Model* model) 
	:Filter(model, "Simple Erosion Filter"){}

	virtual void Render() override;
	virtual nlohmann::json Save() override;
	virtual void Load(nlohmann::json data) override;
	virtual void Apply() override;

	float dimension = 1000.0f;
	float tfac = 1000.0f;
	int iterations = 1024;
};
