#pragma once

#include "Base/NodeEditor/NodeEditor.h"
#include <vector>

class FastNoiseLite;

class NoiseCellularNode : public NodeEditorNode
{
public:


	virtual NodeOutput Evaluate(NodeInputParam input, NodeEditorPin *pin);

	virtual void Load(nlohmann::json data);
	virtual nlohmann::json Save();
	virtual void OnRender();

	NoiseCellularNode();
	~NoiseCellularNode();

	int seed, octaves;
	float frequency, lacunarity, gain, weightedStrength, pingPongStrength, strength, cellularJitter;
	int fractalType, distanceFunc;
	bool useY = false;
	FastNoiseLite *noiseGen;
};