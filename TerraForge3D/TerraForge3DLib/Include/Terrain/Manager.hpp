#pragma once

#include "Base/Core/Core.hpp"
#include "Base/DS/Mesh.hpp"
#include "Terrain/Processor.hpp"

#define TF3D_DEFAULT_TERRAIN_RESOLUTION 128
#define TF3D_DEFAULT_TERRAIN_SCALE 1.0f

namespace TerraForge3D
{
	class ApplicationState;

	namespace Terrain
	{
		class Generator;

		class Manager
		{
		public:
			Manager(ApplicationState* appState);
			~Manager();

			void OnStart();
			void OnEnd();

			void Resize(uint32_t resulution, float scale, float* progress = nullptr);

			void Update();

			void ShowGeneratorSettings();

			void AddGenerator(SharedPtr<Generator> generator);

			inline void SetResolution(uint32_t res) { this->resolution = res; };
			inline void SetScale(float sc) { this->scale = sc; };


			virtual void OnJobDone();

		private:
			void WorkerThread();

			void ProcessGeneratorsOnCPU();
			void ProcessGeneratorsOnVulkan();
			void ProcessGeneratorsOnOpenCL();


		private:
			ApplicationState* appState = nullptr;

			std::vector<SharedPtr<Generator>> generatorsTemp;
			std::vector<SharedPtr<Generator>> generators;
			std::thread workerThread;
			std::atomic<bool> workerThreadRunning = false;
			std::atomic<bool> workerThreadReady = false;
			std::atomic<bool> workerThreadBusy = false;
			std::atomic<bool> isBeingResized = false;
			std::condition_variable condVar;
			std::mutex mutex;

		public:
			SharedPtr<Mesh> mesh;
			SharedPtr<Mesh> meshClone;
			SharedPtr<Terrain::Data> terrainData;


			float scale = TF3D_DEFAULT_TERRAIN_SCALE;
			uint32_t resolution = TF3D_DEFAULT_TERRAIN_RESOLUTION;
			ProcessorDevice processorDevice = ProcessorDevice_CPU;
		};

	}
}