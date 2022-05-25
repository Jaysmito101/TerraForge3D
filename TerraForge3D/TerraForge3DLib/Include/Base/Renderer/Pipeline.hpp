#pragma once
#include "Base/Core/Core.hpp"

namespace TerraForge3D
{
	namespace RendererAPI
	{

		class Pipeline
		{
		public:
			Pipeline() = default;
			~Pipeline() = default;

			virtual void Setup() = 0;
			virtual void Destory() = 0;

			inline Pipeline* SetAutoDestroy(bool value) { this->autoDestory = value; return this; }

			inline bool IsSetup() { return this->isSetup; }

		public:
			static Pipeline* Create();

		protected:
			bool isSetup = false;
			bool autoDestory = true;
		};

	}
}