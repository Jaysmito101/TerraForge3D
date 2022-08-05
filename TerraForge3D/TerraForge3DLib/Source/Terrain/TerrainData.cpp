#include "Terrain/Data.hpp"
#include "TerraForge3D.hpp"

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
			terrainPointData.clear();
		}

		void Data::LoadData()
		{
			this->mesh = appState->terrain.manager->mesh.Get();
			this->meshClone = appState->terrain.manager->meshClone.Get();
		}

		void Data::PrepareForGenerators()
		{
			this->meshClone->Clone(this->mesh);
			if (terrainPointData.size() != tileResolution * tileResolution)
				terrainPointData.resize(tileResolution * tileResolution);
			memset(terrainPointData.data(), 0, sizeof(TerrainPointData) * terrainPointData.size());
		}

		TerrainPointData* Data::Sample(float x, float y)
		{
			TF3D_ASSERT(x <= (tileOffsetX + tileSize) && x >= tileOffsetX, "X Texture Coordinate out of bounds");
			TF3D_ASSERT(y <= (tileOffsetY + tileSize) && y >= tileOffsetY, "Y Texture Coordinate out of bounds");
			uint64_t xP = static_cast<uint64_t>(x * (tileResolution - 1));
			uint64_t yP = static_cast<uint64_t>(y * (tileResolution - 1));
			uint64_t index = xP * tileResolution + yP;
			TF3D_ASSERT(index >= 0 && index < tileResolution, "Index out of bounds");
			return &terrainPointData[index];
		}


	}

}

