#pragma once
#include "Base/Vulkan/Core.hpp"

namespace TerraForge3D
{
    namespace Vulkan
    {
        /*
        * This is more or less same a sVkPhysicalDeviceLimits but only has important data items
        */
        struct PhysicalDeviceLimits
        {
            uint32_t maxImageDimensions1D;
            uint32_t maxImageDimensions2D;
            uint32_t maxImageDimensions3D;
            uint32_t maxImageDimensionsCube;
            uint32_t maxImageArrayLayers;
            uint32_t maxUniformBufferRange;
            uint32_t maxStorageBufferRange;
            uint32_t maxPushConstantsSize;
            uint32_t maxComputeSharedMemorySize;
            uint32_t maxComputeWorkGroupCount[3];
            uint32_t maxComputeWorkGroupInvocations;
            uint32_t maxComputeWorkGroupSize[3];
            uint32_t maxDrawIndexedIndexValue;
            uint32_t maxViewports;
            uint32_t maxViewportDimensions[2];
            float    lineWidthRange[2];
            float    pointSizeRange[2];
        };

        /*
        * This is more or less same as VkPhysicalDeviceFeatures with some items removed
        */
        struct PhysicalDeviceFeatures
        {
            bool imageCubeArray;
            bool geometryShader;
            bool computeShader = true; // Temp
            bool tessellationShader;
            bool logicOp;
            bool multiDrawIndirect;
            bool wideLines;
            bool largePoints;
            bool textureCompressionETC2;
            bool textureCompressionASTC_LDR;
            bool textureCompressionBC;
        };

        /*
        * This holds the properties of the que family
        */
        struct PhysicalDeviceQueueFamilyProperties
        {
            uint32_t queueCount;
            uint32_t index;
            bool compute;
            bool graphics;
            bool transfer;
            bool sparseBinding;
        };

        /*
        * This is more or less same as the VkPhysicalDevice enum formm vulkan.h
        */
        enum PhysicalDeviceType
        {
            PhysicalDeviceType_Unknown          = 0,
            PhysicalDeviceType_IntegratedGPU    = 1,
            PhysicalDeviceType_DiscreteGPU      = 2,
            PhysicalDeviceType_VirtualGPU       = 3,
            PhysicalDeviceType_CPU              = 4,
            PhysicalDeviceType_Count
        };

        /*
        * This class is used to represent a Vulkan Physcial Device.
        * This also automatically retrieves the properties, features
        * so that they can be acccessed more convineintly.
        */
        class PhysicalDevice
        {
        public:
            PhysicalDevice();
            PhysicalDevice(VkPhysicalDevice physicalDevice);
            ~PhysicalDevice();

            // General Utility Functions
            std::string ToString();

            uint32_t GetGraphicsQueueIndex(); /* returns a queue index which has graphics capabilities */
            uint32_t GetComputeQueueIndex(); /* returns a queue index which has compute capabilities */
            uint32_t GetPresentQueueIndex(VkSurfaceKHR surface); /* returns the queue index for presenting*/

        private:
            void GetDeviceProperties();
            void GetDeviceFeatures();
            void GetDeviceQueueFamilyPropeties();
            void CalculateComputeScore();
            void CalculateGraphicsScore();
            PhysicalDeviceQueueFamilyProperties GetQueueFamilyProperty(VkQueueFamilyProperties properties, int index);

        public:
            /* Device Proeprties */
            std::string name = "";
            uint32_t deviceID = 0;
            uint32_t apiVersion = 0;
            uint32_t driverVersion = 0;
            uint32_t vendorID = 0;
            PhysicalDeviceType type = PhysicalDeviceType_Unknown;
            PhysicalDeviceLimits limits;

            /* Device Features */
            PhysicalDeviceFeatures features;

            /* Device Queue Families */
            std::vector<PhysicalDeviceQueueFamilyProperties> queueFamilyProperties;
            uint32_t queueFamilyCount = 0; /* This is not very required as you can just use queueFamilyProperties.size() */

            /* Utulity Shortcuts */
            bool isComputeCapable = false;
            bool isGraphicsCapable = false;
            
            uint64_t computeScore = 0;
            uint64_t graphicsScore = 0;

            bool valid = true;

            /* The VkPhysicalDevice handle */
            VkPhysicalDevice handle = VK_NULL_HANDLE;

        private:

        };

        std::string to_string(PhysicalDeviceType type);
    }
}

