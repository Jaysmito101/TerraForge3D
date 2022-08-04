#pragma once

#include "Base/Core/Core.hpp"

namespace TerraForge3D
{
	namespace RendererAPI
	{

		class SharedStorageBuffer
		{
		public:
			SharedStorageBuffer() = default;
			virtual ~SharedStorageBuffer() = default;

			virtual bool Setup() = 0;
			virtual bool Destroy() = 0;
			virtual bool SetData(void* data, uint64_t size, uint64_t offset = 0) = 0;
			virtual bool GetData(void* data, uint64_t size, uint64_t offset = 0) = 0;

			inline SharedStorageBuffer* SetSize(uint64_t size) { TF3D_ASSERT(!isSetup, "Cannot set size after buffer has been setup"); this->size = size; return this; }
			inline SharedStorageBuffer* SetBinding(uint32_t value) { TF3D_ASSERT(!isSetup, "Cannot set binding after buffer has been setup"); this->binding = value; return this; }
			inline SharedStorageBuffer* SetAutoDestroy(bool value) { this->autoDestory = value; return this; }
			inline SharedStorageBuffer* UseForGraphics() { TF3D_ASSERT(!isSetup, "Cannot change device after buffer has been setup"); this->isGraphicsUse = true; return this; }
			inline SharedStorageBuffer* UseForCompute() { TF3D_ASSERT(!isSetup, "Cannot change device after buffer has been setup"); this->isGraphicsUse = false; return this; }
			inline uint64_t GetSize() { return this->size; }
			inline uint32_t GetBinding() { return this->binding; }
			inline bool IsSetup() { return this->isSetup; }

		public:

			static SharedStorageBuffer* Create();

		protected:
			uint64_t size = 0;
			uint32_t binding = 0;
			bool isGraphicsUse = true;
			bool isSetup = false;
			bool autoDestory = true;
		};

	}

}
