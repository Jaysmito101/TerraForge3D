#pragma once

#include "Base/Vulkan/Core.hpp"
#include "Base/Vulkan/PhysicalDevice.hpp"
#include "Base/Vulkan/LogicalDevice.hpp"

namespace TerraForge3D
{
	namespace Vulkan
	{

		/*
		* This is the vulkan logical device created from a TerraForge3D::Vulkan::PhysicalDevice
		* to be used for compute api management.
		*
		* This takes care of almost everything done using vulkan.
		*/
		class ComputeDevice : public LogicalDevice
		{
		private:
			ComputeDevice(PhysicalDevice& physicalDevice);
			~ComputeDevice();

			virtual void CreateDevice() override;

		public:
			inline static ComputeDevice* Create(PhysicalDevice& physicalDevice) { TF3D_ASSERT(mainInstance == nullptr, "A vulkan compute device already exists"); mainInstance = new ComputeDevice(physicalDevice); return mainInstance; };
			inline static ComputeDevice* Get() { TF3D_ASSERT(mainInstance, "A vulkan compute device does not exist"); return mainInstance; }
			inline static ComputeDevice* Set(ComputeDevice* device) { TF3D_ASSERT(device, "device is NULL"); mainInstance = device; return mainInstance; }
			inline static void Destroy() { TF3D_ASSERT(mainInstance, "A vulkan compute device does not exist"); TF3D_SAFE_DELETE(mainInstance); }

		public:
			VkQueue queue = VK_NULL_HANDLE;		
			std::vector<const char*> extensions;
		private:
			static ComputeDevice* mainInstance;
		};

	}
}
