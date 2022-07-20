#pragma once

#include "Base/Core/Core.hpp"

namespace TerraForge3D
{
	class ApplicationState;
	class Mesh;

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
		
		private:
			ApplicationState* appState = nullptr;
		
		public:
			Mesh* mesh = nullptr;
			Mesh* meshClone = nullptr;
		};

	}
}
