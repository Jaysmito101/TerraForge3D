#include "Terrain/Data.hpp"
#include "Terrain/Manager.hpp"
#include "Data/ApplicationState.hpp"
#include "Base/Base.hpp"

namespace TerraForge3D
{

	namespace Terrain
	{

		Data::Data(ApplicationState* as) 
		{
			this->appState = as;
		}

		Data::~Data()
		{

		}

		void Data::LoadData()
		{
			this->mesh = appState->terrain.manager->mesh.Get();
			this->meshClone = appState->terrain.manager->meshClone.Get();			
		}

	}

}

