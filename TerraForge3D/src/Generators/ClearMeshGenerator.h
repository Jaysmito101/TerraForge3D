#pragma once

#include "OpenCL/ComputeKernel.h"

#include "json.hpp"

struct ApplicationState;

class ClearMeshGenerator
{
public:
	ClearMeshGenerator(ApplicationState* appState, ComputeKernel* kernel);

	virtual void Generate(ComputeKernel* kernels);

	virtual nlohmann::json Save();

	virtual void Load(nlohmann::json data);

	virtual void ShowSettings();

	bool uiActive = false;
	bool useGPU = false;
	bool useGPUForNormals = false;
	double time = 0;
	ApplicationState* appState;
	
};
