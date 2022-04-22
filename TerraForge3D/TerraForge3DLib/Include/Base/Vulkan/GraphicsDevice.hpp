#pragma once

#include "Base/Vulkan/Core.hpp"
#include "Base/Vulkan/PhysicalDevice.hpp"

namespace TerraForge3D
{
	namespace Vulkan
	{
		/*
		* This is the vulkan logical device created from a TerraForge3D::Vulkan::PhysicalDevice 
		* to be used for graphics rendering.
		* 
		* This takes care of almost everything done using vulkan.
		*/
		class GraphicsDevice
		{
		private:
			GraphicsDevice(PhysicalDevice& physicalDevice);
			~GraphicsDevice();

		private:
			void CreateDevice();
			void CreateDescriptorPool();

		public:
			inline static GraphicsDevice* Create(PhysicalDevice& physicalDevice) { TF3D_ASSERT(mainInstance == nullptr, "A vulkan graphics device already exists"); mainInstance = new GraphicsDevice(physicalDevice); return mainInstance; };
			inline static GraphicsDevice* Get() { TF3D_ASSERT(mainInstance, "A vulkan graphics device does not exist"); return mainInstance; }
			inline static GraphicsDevice* Set(GraphicsDevice* device) { TF3D_ASSERT(device, "device is NULL"); mainInstance = device; return mainInstance; }
			inline static void Destroy() { TF3D_ASSERT(mainInstance, "A vulkan graphics device does not exist"); TF3D_SAFE_DELETE(mainInstance); }

		public:
			PhysicalDevice physicalDevice;
			VkQueue graphicsQueue = VK_NULL_HANDLE;
			VkQueue presentQueue = VK_NULL_HANDLE;
			VkDevice handle = VK_NULL_HANDLE;
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			std::vector<const char*> extensions;


			PhysicalDeviceQueueFamilyProperties graphicsQueueProperties;
			PhysicalDeviceQueueFamilyProperties presentQueueProperties;

		private:
			static GraphicsDevice* mainInstance;
		};

	}
}
