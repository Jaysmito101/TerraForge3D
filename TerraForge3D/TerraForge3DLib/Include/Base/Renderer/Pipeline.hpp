#pragma once
#include "Base/Core/Core.hpp"
#include "Base/Renderer/Shader.hpp"

namespace TerraForge3D
{
	namespace RendererAPI
	{
		class FrameBuffer;
		class SharedStorageBuffer;

		class Pipeline
		{
		public:
			Pipeline();
			virtual ~Pipeline();

			virtual void Setup() = 0;
			virtual void Destory() = 0;
			virtual bool Rebuild(FrameBuffer* framebuffer, bool forceRebuild = false) = 0;

			inline Pipeline* SetAutoDestroy(bool value) { this->autoDestory = value; return this; }
			inline Pipeline* AddSharedStorageBuffer(SharedStorageBuffer* buffer) { this->sharedStorageBuffers.push_back(buffer); return this; }

			inline bool IsSetup() { return this->isSetup; }

		public:
			static Pipeline* Create();

			RendererAPI::Shader* shader = nullptr;

		public:
			bool isSetup = false;
			bool autoDestory = true;
			int viewportBegin[2] = {0, 0};
			std::vector<SharedStorageBuffer*> sharedStorageBuffers;
		};

	}
}