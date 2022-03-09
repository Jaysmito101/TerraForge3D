#pragma once

struct ApplicationState;
class Model;
class ClearMeshGenerator;
class CPUNoiseLayersGenerator;
class GPUNoiseLayerGenerator;

class CPUNodeEditor;



#include "Base/OpenCL/ComputeKernel.h"
#include "json/json.hpp"


#include <vector>

#include <atomic>

class MeshGeneratorManager
{
public:

	MeshGeneratorManager(ApplicationState *appState);
	~MeshGeneratorManager();

	void Generate();
	void GenerateSync();
	void ShowSettings();

	void GenerateForTerrain();
	void GenerateForCustomBase();

	void ExecuteKernels();
	void ExecuteCPUGenerators();

	void LoadKernels();

	nlohmann::json Save();
	void Load(nlohmann::json data);

	double time = 0;
	bool windowStat = true;

private:
	ApplicationState *appState;
	Model *tmpModel;
	std::atomic<bool> *isRemeshing;
	ComputeKernel *kernels;
	ClearMeshGenerator *clearMeshGen;
	std::vector<CPUNoiseLayersGenerator *> cpuNoiseLayers;
	std::vector<GPUNoiseLayerGenerator *> gpuNoiseLayers;
	std::vector<CPUNodeEditor *> cpuNodeEditors;
};
