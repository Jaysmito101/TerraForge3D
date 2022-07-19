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

		private:
			ApplicationState* appState = nullptr;
			Mesh* mesh = nullptr;
			Mesh* meshClone = nullptr;
		};

	}
}
