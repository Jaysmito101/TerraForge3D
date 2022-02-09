#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class FastNoiseLite;

class NoiseValueCubicNode : public NodeEditorNode {
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin* pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	NoiseValueCubicNode();
	~NoiseValueCubicNode();

	int seed, octaves;
	float frequency, lacunarity, gain, weightedStrength, pingPongStrength, strength;
	int fractalType;
	FastNoiseLite* noiseGen;
};