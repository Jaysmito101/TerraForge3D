#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class FastNoiseLite;

class NoiseOpenSimplex2Node : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	NoiseOpenSimplex2Node();
	~NoiseOpenSimplex2Node();

	int seed, octaves;
	float frequency, lacunarity, gain, weightedStrength, pingPongStrength, strength, yVal;
	int fractalType;
	FastNoiseLite *noiseGen;
};					