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
			virtual void AddCommand(RendererCommand command, RendererCommandParams params);

			inline void AddCommand(RendererCommand command, void* params) { RendererCommandParams param; param.custom = params; this->AddCommand(command, param); }

		public:
			static NativeRenderer* Create();
			static void Destroy();

		public:
			std::queue<std::pair<RendererCommand, RendererCommandParams>> rendererQueue;

		private:
			static NativeRenderer* mainInstance;
		};

	}

}
