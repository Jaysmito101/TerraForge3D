#pragma once
#include "Base/Core/Core.hpp"
#include "Base/Renderer/Renderer.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{

		class NativeRenderer
		{
		public:
			NativeRenderer() = default;
			virtual ~NativeRenderer() = default;

			virtual void Flush() = 0;
			virtual void AddCommand(RendererCommand command, void* params = nullptr);

		public:
			static NativeRenderer* Create();
			static void Destroy();

		public:
			std::queue<std::pair<RendererCommand, void*>> rendererQueue;

		private:
			static NativeRenderer* mainInstance;
		};

	}

}
