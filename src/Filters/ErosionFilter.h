#pragma once

#include "Filter.h"

class ErosionFilter : public Filter {

public:
	ErosionFilter(Model* model) 
	:Filter(model, "Simple Erosion Filter (CPU)"){}

	virtual void Render() override;
	virtual nlohmann::json Save() override;
	virtual void Load(nlohmann::json data) override;
	virtual void Apply() override;

     int seed = 42;
     int erosionRadius = 3;
     float inertia = .05f; // At zero, water will instantly change direction to flow downhill. At 1, water will never change direction. 
     float sedimentCapacityFactor = 4; // Multiplier for how much sediment a droplet can carry
     float minSedimentCapacity = .01f; // Used to prevent carry capacity getting too close to zero on flatter terrain
     float erodeSpeed = .3f;
     float depositSpeed = .3f;
     float evaporateSpeed = .01f;
     float gravity = 4;
     int maxDropletLifetime = 30;
     int numIterations = 10000;

     float initialWaterVolume = 1;
     float initialSpeed = 1;

    // Indices and weights of erosion brush precomputed for every node
    int** erosionBrushIndices;
    float** erosionBrushWeights;

    int currentSeed;
    int currentErosionRadius;
    int currentMapSize;


};
