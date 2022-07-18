#include "Terrain/Manager.hpp"
#include "Data/ApplicationState.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{

		Manager::Manager(ApplicationState* as)
		{
			this->appState = as;
			this->mesh = new Mesh("MainTerrain");
			this->mesh->Clear();
			this->Resize(TF3D_DEFAULT_TERRAIN_RESOLUTION, TF3D_DEFAULT_TERRAIN_SCALE);
			this->mesh->UploadToGPU();
		}

		Manager::~Manager()
		{
		}

		void Manager::Update()
		{

		}

		void Manager::Resize(uint32_t res, float sc, float* progress)
		{
			//if (progress)
			//	*progress = 0.0f;
			this->isBeingResized = true;
			this->resolution = res;
			this->scale = sc;
			this->mesh->Plane(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), resolution, scale, progress);
			mesh->RecalculateNormals();
			this->isBeingResized = false;
		}

	}
}