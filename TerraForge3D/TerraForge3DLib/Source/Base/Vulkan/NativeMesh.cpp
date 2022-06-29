#include "Base/Renderer/NativeMesh.hpp"
#include "Base/Vulkan/NativeMesh.hpp"
#include "Base/DS/Mesh.hpp"

#ifdef TF3D_VULKAN_BACKEND
namespace TerraForge3D
{

	namespace Vulkan
	{
		NativeMesh::NativeMesh()
		{
			this->vertexBuffer = new Buffer();
			this->indexBuffer = new Buffer();
		}

		NativeMesh::~NativeMesh()
		{
			if (autoDestroy)
				Destroy();
			
			TF3D_SAFE_DELETE(vertexBuffer);
			TF3D_SAFE_DELETE(indexBuffer);
		}

		bool NativeMesh::Setup()
		{
			TF3D_ASSERT(!isSetup, "Mesh has already been setup call destory first");
			TF3D_ASSERT(indexCount > 0 && vertexCount > 0, "Invalid vertex or index cound provided");
			TF3D_ASSERT(indexCount % 3 == 0, "Index count must be a multiple of 3");

			vertexBuffer->usageflags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			vertexBuffer->bufferSize = sizeof(Vertex) * vertexCount;
			vertexBuffer->Setup();

			indexBuffer->usageflags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			indexBuffer->bufferSize = sizeof(uint32_t) * indexCount;
			indexBuffer->Setup();


			isSetup = true;

			return true;
		}

		bool NativeMesh::Destroy()
		{
			TF3D_ASSERT(isSetup, "Mesh has not yet been setup call setup first");

			vertexBuffer->Destory();
			indexBuffer->Destory();

			isSetup = false;

			return true;
		}

		bool NativeMesh::UploadData(void* vertices, void* indices)
		{
			TF3D_ASSERT(isSetup, "Mesh has not yet been setup call setup first");
			TF3D_ASSERT(vertices, "Vertices is null");
			TF3D_ASSERT(indices, "Indices is null");
			vertexBuffer->Map();
			vertexBuffer->SetData(vertices, sizeof(Vertex) * vertexCount);
			vertexBuffer->Unmap();
			indexBuffer->Map();
			indexBuffer->SetData(indices, sizeof(uint32_t) * indexCount);
			indexBuffer->Unmap();
			return true;
		}
	}

}
#endif