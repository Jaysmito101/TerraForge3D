#pragma once

#include "json.hpp"
#include <string>

class FastNoiseLite;

struct NoiseLayerInput
{
	float x;
	float y;
	float z;
};

class NoiseLayer
{
public:
	NoiseLayer();
	~NoiseLayer();

	nlohmann::json Save();
	void Load(nlohmann::json data);
	float Evaluate(NoiseLayerInput input);
	void Render(int index);

	std::string name;
	const char *noiseTypeStr, *distFuncStr, *fractalTypeStr;
	int seed, octaves;
	float frequency, lacunarity, gain, weightedStrength, pingPongStrength, strength, cellularJitter;
	int fractalType, distanceFunc, noiseType;
	FastNoiseLite *noiseGen;
	float offset[3];
	bool enabled;
};
