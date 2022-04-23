#pragma once
#include "Base/Core/Core.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{

		class Context
		{
		public:
			Context();
			~Context();

			/*
			* This functions finishes all the ongoing oprtation with the context
			*/
			virtual void WaitForIdle() = 0;

		public:
			static Context* Create();
			static Context* Get();
			static Context* Set(Context* context);
			static void Destroy();

		private:
			static Context* mainInstance;
		};

	}

}
