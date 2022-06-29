#pragma once
#include "Base/Vulkan/Core.hpp"
#include "Base/Renderer/NativeMesh.hpp"
#include "Base/Vulkan/Buffer.hpp"

#ifdef TF3D_VULKAN_BACKEND
namespace TerraForge3D
{

	namespace Vulkan
	{

		class NativeMesh : public RendererAPI::NativeMesh
		{
		public:
			NativeMesh();
			~NativeMesh();

			virtual bool Setup() override;
			virtual bool Destroy() override;
			virtual bool UploadData(void* vertices, void* indices) override;

		public:
			Buffer* vertexBuffer = nullptr;
			Buffer* indexBuffer = nullptr;
		};

	}

}
#endif