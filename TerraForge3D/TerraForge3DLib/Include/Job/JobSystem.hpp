#pragma once
#include "Base/Core/Core.hpp"

namespace TerraForge3D
{
	class ApplicationState;


	namespace JobSystem
	{

		class JobSystem
		{
		public:
			JobSystem(ApplicationState* appState);
			~JobSystem();

			void Update();

		public:

		private:
			ApplicationState* appState;
		};

	}

}
