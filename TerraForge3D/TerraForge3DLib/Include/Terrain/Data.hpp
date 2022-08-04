#pragma once

#include "Base/Base.hpp"

namespace TerraForge3D
{
	class ApplicationState;
	class Mesh;

	struct TerrainPointData
	{
		glm::vec4 a = glm::vec4(0.0f);
		glm::vec4 b = glm::vec4(0.0f);
		glm::vec4 c = glm::vec4(0.0f);
		glm::vec4 d = glm::vec4(0.0f);
	};

	namespace Terrain
	{

		class Manager;

		class Data
		{
		public:
			Data(ApplicationState* appState);
			~Data();

			void LoadData();

			void PrepareForGenerators();

			TerrainPointData* Sample(float x, float y);
		
		private:
			ApplicationState* appState = nullptr;
		
		public:
			Mesh* mesh = nullptr;
			Mesh* meshClone = nullptr;
			std::vector<TerrainPointData> terrainPointData;
			float tileSize = 1.0f;
			float tileOffsetX = 0.0f;
			float tileOffsetY = 0.0f;
			uint64_t tileResolution = 128;
		};

	}
}
