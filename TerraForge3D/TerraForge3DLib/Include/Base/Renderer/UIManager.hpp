#pragma once
#include "Base/Renderer/UIManager.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{

		class UIManager
		{
		public:
			UIManager();
			~UIManager();

			virtual void Begin() = 0;
			virtual void End() = 0;

		public:
			static UIManager* Create();
			static UIManager* Get();
			static UIManager* Set(UIManager* manager);
			static void Destory();


		public:
			float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };

		private:
			static UIManager* mainInstance;
		};


	}

}
