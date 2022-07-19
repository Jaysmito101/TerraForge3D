#pragma once

#include "Base/Core/Core.hpp"
#include "Base/DS/Mesh.hpp"

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

			void Resize(uint32_t resulution, float scale, float* progress = nullptr);

			void Update();

			void ShowGeneratorSettings();

			void AddGenerator(SharedPtr<Generator> generator);

			inline void SetResolution(uint32_t res) { this->resolution = res; };
			inline void SetScale(float sc) { this->scale = sc; };



		private:
			ApplicationState* appState = nullptr;

			std::vector<SharedPtr<Generator>> generators;

		public:
			SharedPtr<Mesh> mesh;
			SharedPtr<Mesh> meshClone;

			std::atomic<bool> isBeingResized = false;

			float scale = TF3D_DEFAULT_TERRAIN_SCALE;
			uint32_t resolution = TF3D_DEFAULT_TERRAIN_RESOLUTION;
		};

	}
}