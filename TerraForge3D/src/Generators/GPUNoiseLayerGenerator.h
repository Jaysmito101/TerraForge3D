#pragma once

#include "OpenCL/ComputeKernel.h"

#include "json.hpp"

struct ApplicationState;


#pragma pack(push, 1)
struct  GPUNoiseLayer
{
	float octaves = 3.0f;
	float fractal = 0.0f;
	float frequency = 1.0f;
	float lacunarity = 2.0f ;
	float gain = 0.5f ;
	float weightedStrength = 0.2f;
	float pingPongStrength = 0.2f ;
	float strength = 1.0f ;
	float domainWrapDepth = 0.0f ;
	float offsetX = 0.0f, offsetY = 0.0f, offsetZ = 0.0f, offsetW = 0.0f;

	float valueX, valueY, valueZ, valueW; // Ony for internal use on GPU side
};
#pragma pack(pop)


class GPUNoiseLayerGenerator
{
public:
	GPUNoiseLayerGenerator(ApplicationState *appState, ComputeKernel *kernel);

	virtual void Generate(ComputeKernel *kernels);

	virtual nlohmann::json Save();

	virtual nlohmann::json SaveNoiseLayer(GPUNoiseLayer nl);

	virtual GPUNoiseLayer LoadNoiseLayer(nlohmann::json data);

	virtual void Load(nlohmann::json data);

	virtual void ShowSetting(int i);

	void Update();



	bool windowStat = false;
	bool uiActive = false;
	bool enabled = true;
	double time = 0;
	int localSize = 256;
	int vc = 0;
	ApplicationState *appState;
	std::string uid = "";
	std::string name = "";
	std::vector<GPUNoiseLayer> noiseLayers;
};
