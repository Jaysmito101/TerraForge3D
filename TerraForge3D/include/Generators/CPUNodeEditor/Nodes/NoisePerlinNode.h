#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class FastNoiseLite;

class NoisePerlinNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	NoisePerlinNode();
	~NoisePerlinNode();

	int seed, octaves;
	float frequency, lacunarity, gain, weightedStrength, pingPongStrength, strength;
	int fractalType;
	FastNoiseLite *noiseGen;
};