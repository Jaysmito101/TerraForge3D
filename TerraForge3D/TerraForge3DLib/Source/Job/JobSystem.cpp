#include "Job/JobSystem.hpp"
#include "Data/ApplicationState.hpp"

namespace TerraForge3D
{

	namespace JobSystem
	{

		

		JobSystem::JobSystem(ApplicationState* appState)
		{
			this->appState = appState;
		}

		JobSystem::~JobSystem()
		{
		}

		void JobSystem::Update()
		{
		}

	}

}