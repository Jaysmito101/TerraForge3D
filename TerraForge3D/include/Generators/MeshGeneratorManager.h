#pragma once

class ApplicationState;
class CPUNoiseLayersGenerator;
class GPUNoiseLayerGenerator;
class CPUNodeEditor;


#include "Generators/CPUGeneratorWorker.h"
#include "json/json.hpp"

#include <vector>
#include <atomic>

class MeshGeneratorManager
{
public:

	MeshGeneratorManager(ApplicationState *appState);
	~MeshGeneratorManager();

	void ShowSettings();


	nlohmann::json Save();
	void Load(nlohmann::json data);

	double time = 0;
	bool windowStat = true;

	inline std::vector<CPUNoiseLayersGenerator*>& GetCPUNoiseLayers() { return this->cpuNoiseLayers; }
	inline std::vector<GPUNoiseLayerGenerator*>& GetGPUNoiseLayers() { return this->gpuNoiseLayers; }
	inline std::vector<CPUNodeEditor*>& GetCPUNodeEditors() { return this->cpuNodeEditors; }

private:
	ApplicationState *appState;
	std::vector<GPUNoiseLayerGenerator *> gpuNoiseLayers;
	std::vector<CPUNoiseLayersGenerator *> cpuNoiseLayers;
	std::vector<CPUNodeEditor *> cpuNodeEditors;
};
