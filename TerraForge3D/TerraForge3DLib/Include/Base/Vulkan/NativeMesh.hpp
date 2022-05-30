#pragma once
#include "Base/Vulkan/Core.hpp"
#include "Base/Renderer/NativeMesh.hpp"

#ifdef TF3D_VULKAN_BACKEND
namespace TerraForge3D
{

	namespace Vulkan
	{

		class NativeMesh : public RendererAPI::NativeMesh
		{
		public:
			NativeMesh() = default;
			~NativeMesh();

			virtual bool Setup() override;
			virtual bool Destroy() override;
			virtual bool UploadData(void* vertices, void* indices) override;

		public:
			// TODO: Implement
		};

	}

}
#endif