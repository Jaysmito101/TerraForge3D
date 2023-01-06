#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class FastNoiseLite;

class NoiseValueNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	NoiseValueNode();
	~NoiseValueNode();

	int seed, octaves;
	float frequency, lacunarity, gain, weightedStrength, pingPongStrength, strength, yVal;
	int fractalType;
	FastNoiseLite *noiseGen;
};