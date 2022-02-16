#pragma once

struct ApplicationState;

#include "NoiseLayers/LayeredNoiseManager.h"
#include "GeneratorMask.h"

#include <string>

#include "json.hpp"

class CPUNoiseLayersGenerator
{
public:
	CPUNoiseLayersGenerator(ApplicationState* appState);
	~CPUNoiseLayersGenerator();

	nlohmann::json Save();
	void Load(nlohmann::json data);

	void ShowSetting(int id);

	void Update();

	float EvaluateAt(float x, float y, float z);

	bool windowStat = false;
	bool uiActive = false;
	bool enabled = true;
	double time = 0;
	std::string uid;
	std::string name;
	ApplicationState* appState;
	LayeredNoiseManager* noiseManager;
	GeneratorMaskManager* maskManager;
};