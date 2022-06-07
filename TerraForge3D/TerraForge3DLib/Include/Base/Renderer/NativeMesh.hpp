#pragma once
#include "Base/Core/Core.hpp"
#include "Base/Renderer/Renderer.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{

		class NativeMesh
		{
		public:
			NativeMesh() = default;
			virtual ~NativeMesh() = default;
			
			inline size_t GetIndexCount() {  return indexCount; };
			inline size_t GetVertexCount() { return vertexCount; };
			inline bool IsSetup() { return isSetup; }

			inline NativeMesh* SetIndexCount(size_t count) { TF3D_ASSERT(!isSetup, "Cannot set params after setup call Destory first"); this->indexCount = count; return this; }
			inline NativeMesh* SetVertexCount(size_t count) { TF3D_ASSERT(!isSetup, "Cannot set params after setup call Destory first"); this->vertexCount = count; return this; }
			inline NativeMesh* SetAutoDestory(bool value) { TF3D_ASSERT(!isSetup, "Cannot set params after setup call Destory first"); this->autoDestroy = value; return this; }

			virtual bool Setup() = 0;
			virtual bool Destroy() = 0;
			virtual bool UploadData(void* vertices, void* indices) = 0;

		public:
			static NativeMesh* Create();

		public:

		protected:
			size_t indexCount = 0;
			size_t vertexCount = 0;
			bool isSetup = false;
			bool autoDestroy = true;
		};

	}

}
