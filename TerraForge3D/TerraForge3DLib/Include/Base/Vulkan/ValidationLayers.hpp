#pragma once

#ifdef TF3D_DEBUG // Enable Validation Layers only in Debug Builds

#include "Base/Vulkan/Core.hpp"

namespace TerraForge3D
{
	namespace Vulkan
	{
		/*
		* This calss Manages all the validation layers, also sets up the 
		* Debug Messenger
		*/
		class ValidationLayers
		{
		public:

			/*
			* Check if validation layers are supported by the system
			*/
			static bool CheckSupport();

			/*
			* Returns the number of validation layers needed
			*/
			static uint32_t GetLayerCount();

			/*
			* Returns the names of the validation layers
			*/
			static const char** GetLayerNames();

			/*
			* Fills up the VkDebugUtilsMessengerCreateInfoEXT struct
			*/
			static VkDebugUtilsMessengerCreateInfoEXT* PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo);

			/*
			* Createsthe Debug Messenger
			*/
			static void CreateDebugMessenger(VkInstance instance);

			/*
			* Destroys the Debug Messenger
			*/
			static void DestroyDebugMessenger(VkInstance instance);

		private:
			static VkDebugUtilsMessengerEXT debugMessenger;
		};

	}
}

#endif
