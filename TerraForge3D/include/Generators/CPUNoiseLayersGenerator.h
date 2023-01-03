#pragma once

struct ApplicationState;

#include "NoiseLayers/LayeredNoiseManager.h"
#include "GeneratorMask.h"

#include <string>

#include "json/json.hpp"

class CPUNoiseLayersGenerator
{
public:
	CPUNoiseLayersGenerator(ApplicationState *appState);
	~CPUNoiseLayersGenerator();

	nlohmann::json Save();
	void Load(nlohmann::json data);

	void ShowSetting(int id);

	void Update();

	float EvaluateAt(float x, float y, float z) const;

	bool windowStat = false;
	bool uiActive = false;
	bool enabled = true;
	double time = 0;
	std::string uid;
	std::string name;

	int setMode = 0; // 0 = set, 1 = add, 2 = sub, 3 = mul
	
	ApplicationState *appState;
	LayeredNoiseManager *noiseManager;
	GeneratorMaskManager *maskManager;
};