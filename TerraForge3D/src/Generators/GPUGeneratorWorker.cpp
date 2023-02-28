#include "Generators/GPUGeneratorWorker.h"
#include "Generators/MeshGeneratorManager.h"

#include "Utils/Utils.h"
#include "Base/Base.h"
#include "Data/ApplicationState.h"

#include "Generators/CPUNoiseLayersGenerator.h"
#include "Generators/GPUNoiseLayerGenerator.h"
#include "Base/NodeEditor/NodeEditor.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"

#include "Base/UIFontManager.h"

#include "Profiler.h"
#include "Platform.h"


GPUGeneratorWorker::GPUGeneratorWorker(ApplicationState* appState, int id)
{
	this->appState = appState;
	this->id = id;
	this->current_state = GPUGeneratorWorkerState_Dead;
	this->SetupOpenCL();
	// this->m_OpenCLContextToUse = id % m_OpenCLContexts.size(); // For future maybe
	this->m_OpenCLContextToUse = 0;
}

GPUGeneratorWorker::~GPUGeneratorWorker()
{
	Quit();
}

void GPUGeneratorWorker::AddJob(int tx, int ty)
{
	if (this->current_state == GPUGeneratorWorkerState_Dead) return;
	jobs_sizes.push(std::make_pair(tx, ty));
	this->current_state = GPUGeneratorWorkerState_Working;
}

void GPUGeneratorWorker::Start()
{
	if (this->current_state != GPUGeneratorWorkerState_Dead) return;
	this->worker_th = std::thread(std::bind(&GPUGeneratorWorker::Worker, this));
	this->worker_th.detach();
}

void GPUGeneratorWorker::Quit()
{
	this->request_quit = true;
	this->WaitForFinish();
	if (worker_th.joinable()) worker_th.join();
}

bool GPUGeneratorWorker::HasFinishedJob()
{
	return current_state == GPUGeneratorWorkerState_Waiting && jobs_sizes_finished.size() > 0 && jobs_sizes.size() == 0;
}

void GPUGeneratorWorker::WaitForFinish()
{
	while (this->jobs_sizes.size() > 0 || this->current_state == GPUGeneratorWorkerState_Working)
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10ms);
	}
}

void GPUGeneratorWorker::Worker()
{
	//std::cout << "GPU Worker [ID:" << id << "] started\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	auto activeGPUWorkerThreadsCount = this->appState->workManager->GetGPUGeneratorWorkerCount();
	while (!this->request_quit)
	{
		this->current_state = GPUGeneratorWorkerState_Waiting;
		activeGPUWorkerThreadsCount = this->appState->workManager->GetGPUGeneratorWorkerCount();
		while (!this->request_quit && !(jobs_sizes.size() > 0)) { using namespace std::chrono_literals; std::this_thread::sleep_for((id < activeGPUWorkerThreadsCount) ? 10ms : 1000ms); }
		if (this->request_quit) break;
		this->jobs_sizes_finished.clear();
		this->current_state = GPUGeneratorWorkerState_Working;
		this->progress = 0.0f;

		auto job_sz = jobs_sizes.front(); jobs_sizes.pop();
		this->currentX = job_sz.first;
		this->currentY = job_sz.second;

		int tileSize = appState->workManager->GetWorkResolution();
		size_t tileDataSize = (tileSize * tileSize * 4) * sizeof(float);


		auto& gpuNoiseLayers = appState->meshGenerator->GetGPUNoiseLayers();
		 
		START_PROFILER();

		for (int i = 0; i < 2; i++)
		{ 
			m_OpenCLContexts[m_OpenCLContextToUse]->CreateBuffer("data_layer_" + std::to_string(i), CL_MEM_READ_WRITE, tileDataSize);
			// m_OpenCLContexts[m_OpenCLContextToUse]->WriteBuffer("data_layer_" + std::to_string(i), true, 0, tileDataSize, appState->mainMap.currentTileDataLayers[i]->GetTilePointer(job_sz.first, job_sz.second));
		}
		 
		for (int i = 0; i < gpuNoiseLayers.size(); i++)
			if (gpuNoiseLayers[i]->enabled) 
				gpuNoiseLayers[i]->Generate(m_OpenCLContexts[m_OpenCLContextToUse], job_sz.first, job_sz.second);

		for (int i = 0; i < 2; i++)
			m_OpenCLContexts[m_OpenCLContextToUse]->ReadBuffer("data_layer_" + std::to_string(i), false , 0, tileDataSize, appState->mainMap.currentTileDataLayers[i]->GetTilePointer(job_sz.first, job_sz.second));

		END_PROFILER(jobTime);
		this->progress = 1.0f;
		this->jobs_sizes_finished.push_back(job_sz);
	}
	this->current_state = GPUGeneratorWorkerState_Dead;
}

void GPUGeneratorWorker::SetupOpenCL()
{
	// Get Platforms
	auto platforms = OpenCLPlatform::GetPlatforms();
	m_OpenCLContexts.clear();
	for (auto& platform : platforms)
	{
		// Get Devices
		auto devices = platform.GetDevices();
		for (auto& device : devices)
		{
			// Create Context
			auto context = new OpenCLContext(device);
			m_OpenCLContexts.push_back(context);
			PrepareOpenCLContext(context);
		}
	}
}

void GPUGeneratorWorker::PrepareOpenCLContext(OpenCLContext* context)
{
	bool tmp = false;
	std::string source = ReadShaderSourceFile(appState->constants.kernelsDir + PATH_SEPARATOR "generators" PATH_SEPARATOR "generators.cl", &tmp);
	context->AddSource(source);
	context->BuildProgram("-I" + appState->globals.kernelsIncludeDir + " -cl-fast-relaxed-math -cl-mad-enable");
	context->AddKernel("process_map_noise_layer");

}
