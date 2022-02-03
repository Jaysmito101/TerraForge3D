#pragma once

#include "Filter.h"

class AdvancedErosionFilter : public Filter {

public:
	AdvancedErosionFilter(Model* model) 
	:Filter(model, "Wind Erosion Filter (GPU)"){}
 
	virtual void Render() override;
	virtual nlohmann::json Save() override;
	virtual void Load(nlohmann::json data) override;
	virtual void Apply() override;

    int seed = 42;
    int localWorkSize = 1;

};
