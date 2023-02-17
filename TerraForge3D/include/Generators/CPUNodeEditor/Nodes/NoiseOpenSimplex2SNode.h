#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class FastNoiseLite;

class NoiseOpenSimplex2SNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	NoiseOpenSimplex2SNode();
	~NoiseOpenSimplex2SNode();

	int seed, octaves;
	float frequency, lacunarity, gain, weightedStrength, pingPongStrength, strength, yVal;
	int fractalType;
	FastNoiseLite *noiseGen;
};