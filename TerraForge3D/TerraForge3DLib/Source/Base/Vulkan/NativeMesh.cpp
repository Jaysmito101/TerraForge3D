#include "Base/Renderer/NativeMesh.hpp"
#include "Base/Vulkan/NativeMesh.hpp"

#ifdef TF3D_VULKAN_BACKEND
namespace TerraForge3D
{

	namespace Vulkan
	{
		NativeMesh::~NativeMesh()
		{
			if (autoDestroy)
				Destroy();
		}

		bool NativeMesh::Setup()
		{
			return false;
		}

		bool NativeMesh::Destroy()
		{
			return false;
		}

		bool NativeMesh::UploadData(void* vertices, void* indices)
		{
			return false;
		}
	}

}
#endif