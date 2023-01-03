#pragma once

class ApplicationState;
class Model;
class ClearMeshGenerator;
class CPUNoiseLayersGenerator;
class GPUNoiseLayerGenerator;

class CPUNodeEditor;



#include "Base/OpenCL/ComputeKernel.h"
#include "Generators/CPUGeneratorWorker.h"
#include "json/json.hpp"

#define CPU_GENERATOR_WORKER_COUNT_MAX 16


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
	std::vector<GPUNoiseLayerGenerator *> gpuNoiseLayers;
	std::vector<CPUNoiseLayersGenerator *> cpuNoiseLayers;
	std::vector<CPUNodeEditor *> cpuNodeEditors;
	int cpuWorkerCount = 0;
	CPUGeneratorWorker* cpuGeneratorWorkers[CPU_GENERATOR_WORKER_COUNT_MAX];
};
