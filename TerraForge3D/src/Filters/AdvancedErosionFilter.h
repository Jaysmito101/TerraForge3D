#pragma once

#include "Base/OpenCL/ComputeKernel.h"

#include "Filter.h"

#pragma pack(push, 1)
struct WindErosionParticle
{
	float dt = 0.25f;
	float suspension = 0.0001f;
	float abrasion = 0.0001f;
	float roughness = 0.005f;
	float settling = 0.01f;
	float sediment = 0.25f;
	float height = 0.0f;	
	float index = 0.0f;
	float seed = 0.0f;

	float dimX = 0.0f;
	float dimY = 0.0f;

	float posX = 0.0f;
	float posY = 0.0f;

	float pspeedX = 1.0f;
	float pspeedY = 0.0f;
	float pspeedZ = 1.0f;
	float pspeedW = 0.0f;

	float speedX = 1.0f;
	float speedY = 0.0f;
	float speedZ = 1.0f;
	float speedW = 0.0f;
};
#pragma pack(pop)

class AdvancedErosionFilter : public Filter {

public:
	AdvancedErosionFilter(Model* model);
 
	virtual void Render() override;
	virtual nlohmann::json Save() override;
	virtual void Load(nlohmann::json data) override;
	virtual void Apply() override;

	int seed = 42;
	int particles = 500;
	int localWorkSize = 1;
	ComputeKernel kernels;
	WindErosionParticle particle;

};
