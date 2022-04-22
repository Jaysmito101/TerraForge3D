#include "Base/Vulkan/PhysicalDevice.hpp"

namespace TerraForge3D
{
    namespace Vulkan
    {
        PhysicalDevice::PhysicalDevice()
        {
        }

        PhysicalDevice::PhysicalDevice(VkPhysicalDevice physicalDevice)
        {
            handle = physicalDevice;
            GetDeviceProperties();
            GetDeviceFeatures();
            GetDeviceQueueFamilyPropeties();
            CalculateComputeScore();
            CalculateGraphicsScore();
            if(computeScore > 100000) 
                isComputeCapable = true;
            if(graphicsScore > 50000)
                isGraphicsCapable = true;

            valid = true;
        }

        PhysicalDevice::~PhysicalDevice()
        {
        }

        void PhysicalDevice::GetDeviceProperties()
        {
            VkPhysicalDeviceProperties vkProperties;
            vkGetPhysicalDeviceProperties(handle, &vkProperties);
            name = std::string(vkProperties.deviceName);
            apiVersion = vkProperties.apiVersion;
            driverVersion = vkProperties.driverVersion;
            vendorID = vkProperties.vendorID;
            deviceID = vkProperties.deviceID;
            type = static_cast<PhysicalDeviceType>(vkProperties.deviceType); /* We can do this as VkDeviceType maps exactly to PhysicalDeviceType */

            limits.maxImageDimensions1D = vkProperties.limits.maxImageDimension1D;
            limits.maxImageDimensions2D = vkProperties.limits.maxImageDimension2D;
            limits.maxImageDimensions3D = vkProperties.limits.maxImageDimension3D;
            limits.maxImageDimensionsCube = vkProperties.limits.maxImageDimensionCube;
            limits.maxImageArrayLayers = vkProperties.limits.maxImageArrayLayers;
            limits.maxUniformBufferRange = vkProperties.limits.maxUniformBufferRange;
            limits.maxStorageBufferRange = vkProperties.limits.maxStorageBufferRange;
            limits.maxPushConstantsSize = vkProperties.limits.maxPushConstantsSize;
            limits.maxComputeSharedMemorySize = vkProperties.limits.maxComputeSharedMemorySize;
            limits.maxComputeWorkGroupCount[0] = vkProperties.limits.maxComputeWorkGroupCount[0];
            limits.maxComputeWorkGroupCount[1] = vkProperties.limits.maxComputeWorkGroupCount[1];
            limits.maxComputeWorkGroupInvocations = vkProperties.limits.maxComputeWorkGroupInvocations;
            limits.maxComputeWorkGroupSize[0] = vkProperties.limits.maxComputeWorkGroupSize[0];
            limits.maxComputeWorkGroupSize[1] = vkProperties.limits.maxComputeWorkGroupSize[1];
            limits.maxComputeWorkGroupSize[2] = vkProperties.limits.maxComputeWorkGroupSize[2];
            limits.maxDrawIndexedIndexValue = vkProperties.limits.maxDrawIndexedIndexValue;
            limits.maxViewports = vkProperties.limits.maxViewports;
            limits.maxViewportDimensions[0] = vkProperties.limits.maxViewportDimensions[0];
            limits.maxViewportDimensions[1] = vkProperties.limits.maxViewportDimensions[1];
            limits.lineWidthRange[0] = vkProperties.limits.lineWidthRange[0];
            limits.lineWidthRange[1] = vkProperties.limits.lineWidthRange[1];
            limits.pointSizeRange[0] = vkProperties.limits.pointSizeRange[0];
            limits.pointSizeRange[1] = vkProperties.limits.pointSizeRange[1];
        }

        void PhysicalDevice::GetDeviceFeatures()
        {
            VkPhysicalDeviceFeatures vkFeatures;
            vkGetPhysicalDeviceFeatures(handle, &vkFeatures);
            features.imageCubeArray = vkFeatures.imageCubeArray;
            features.geometryShader = vkFeatures.geometryShader;
            features.tessellationShader = vkFeatures.tessellationShader;
            features.logicOp = vkFeatures.logicOp;
            features.multiDrawIndirect = vkFeatures.multiDrawIndirect;
            features.wideLines = vkFeatures.wideLines;
            features.largePoints = vkFeatures.largePoints;
            features.textureCompressionETC2 = vkFeatures.textureCompressionETC2;
            features.textureCompressionASTC_LDR = vkFeatures.textureCompressionASTC_LDR;
            features.textureCompressionBC = vkFeatures.textureCompressionBC;
        }

        void PhysicalDevice::GetDeviceQueueFamilyPropeties()
        {
            vkGetPhysicalDeviceQueueFamilyProperties(handle, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(handle, &queueFamilyCount, queueFamilies.data());

            queueFamilyProperties.reserve(queueFamilyCount);
            uint32_t i = 0;
            for (auto& queueFamilyProperty : queueFamilies)
            {
                queueFamilyProperties.push_back(GetQueueFamilyProperty(queueFamilyProperty, i));
                i++;
            }
        }

        PhysicalDeviceQueueFamilyProperties PhysicalDevice::GetQueueFamilyProperty(VkQueueFamilyProperties props, int index)
        {
            PhysicalDeviceQueueFamilyProperties properties;
            properties.index = index;
            properties.queueCount = props.queueCount;
            properties.compute = props.queueFlags & VK_QUEUE_COMPUTE_BIT;
            properties.graphics = props.queueFlags & VK_QUEUE_GRAPHICS_BIT;
            properties.transfer = props.queueFlags & VK_QUEUE_TRANSFER_BIT;
            properties.sparseBinding = props.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
            return properties;
        }

        /*
        * Calculate a score to represent how capable this device is as a compute device.
        */
        void PhysicalDevice::CalculateComputeScore()
        {
            computeScore = 0;
            // check if compute queue is present
            for (auto& queueFamily : queueFamilyProperties)
            {
                if (queueFamily.compute)
                {
                    computeScore += 10;
                    break;
                }
            }
            if (computeScore == 0)
                return; // no compute queue, no point in continuing
            // first check if the device supports compute
            if (features.computeShader)
                computeScore += 10;
            else
                return; // no compute support, no point in checking the rest
            // score is more with more que families
            computeScore += queueFamilyProperties.size() * 10;
            // more recent drivers are better
            // computeScore += (driverVersion / 100) * 10;
            // more recent api versions are better
            // computeScore += (apiVersion / 100) * 10;
            // GPU preference for compute Discrete > Integrated > Virtual > CPU
            if (type == PhysicalDeviceType_DiscreteGPU)
                computeScore += 1000;
            else if (type == PhysicalDeviceType_IntegratedGPU)
                computeScore += 500;
            else if (type == PhysicalDeviceType_VirtualGPU)
                computeScore += 100;
            else if (type == PhysicalDeviceType_CPU)
                computeScore -= 10; // cpu is not good for compute
            // more shared memory is better
            computeScore += limits.maxComputeSharedMemorySize / (1 << 20);
            // more compute work group size is better
            computeScore += limits.maxComputeWorkGroupSize[0] + limits.maxComputeWorkGroupSize[1] + limits.maxComputeWorkGroupSize[2];
            // more compute work group count is better
            computeScore += limits.maxComputeWorkGroupCount[0] + limits.maxComputeWorkGroupCount[1] + limits.maxComputeWorkGroupCount[2];
            // more compute work group invocations is better
            computeScore += limits.maxComputeWorkGroupInvocations / 10;
            // higher image array layers is better
            computeScore += limits.maxImageArrayLayers / 10;
            // higher max image dimension is better
            computeScore += limits.maxImageDimensions2D / 10;
            // higher storage buffer range is better
            computeScore += limits.maxStorageBufferRange / (1 << 20);
            // higher uniform buffer range is better
            computeScore += limits.maxUniformBufferRange / (1 << 20);
        }

        /*
        * Calculate a score to represent how capable this device is as a graphics device.
        */
        void PhysicalDevice::CalculateGraphicsScore()
        {
            graphicsScore = 0;
            // for same thing compute score is multiplied by 10
            // but for graphics it is multiplied by 5 as compute is more important
            // firct check is graphics queue is present
            for (auto& queueFamily : queueFamilyProperties)
            {
                if (queueFamily.graphics)
                {
                    graphicsScore += 10;
                    break;
                }
            }
            if (graphicsScore == 0)
                return; // no graphics queue, no point in checking the rest
            // first check if the device supports geometry shaders
            if (features.geometryShader)
                graphicsScore += 10;
            // score is more with more queue families
            graphicsScore += queueFamilyProperties.size() * 5;
            // more recent drivers are better
            // graphicsScore += (driverVersion / 100) * 5;
            // more recent api versions are better
            // graphicsScore += (apiVersion / 100) * 5;
            // GPU preference for graphics Discrete > Integrated > Virtual > CPU
            if (type == PhysicalDeviceType_DiscreteGPU)
                graphicsScore += 100;
            else if (type == PhysicalDeviceType_IntegratedGPU)
                graphicsScore += 50;
            else if (type == PhysicalDeviceType_VirtualGPU)
                graphicsScore += 10;
            else if (type == PhysicalDeviceType_CPU)
                graphicsScore -= 100; // cpu is not good for graphics
            // higher storage buffer range is better
            graphicsScore += limits.maxStorageBufferRange / 100;
            // higher uniform buffer range is better
            graphicsScore += limits.maxUniformBufferRange / 100;
        }

        std::string PhysicalDevice::ToString()
        {
            std::stringstream ss;
            /* Device Properties */
            ss << "PhysicalDevice\t\t: " << name << std::endl;
            ss << "API Version\t\t: " << apiVersion << std::endl;
            ss << "Driver Version\t\t: " << driverVersion << std::endl;
            ss << "Vendor ID\t\t: " << vendorID << std::endl;
            ss << "Device ID\t\t: " << deviceID << std::endl;
            ss << "Type\t\t\t: " << to_string(type) << std::endl;
            ss << "Compute Score\t\t: " << computeScore << std::endl;
            ss << "Graphics Score\t\t: " << graphicsScore << std::endl;
            ss << std::endl;

            /* Device Limits */
            ss << "Limits:- " << std::endl;
            ss << "\tMax Image Dimensions 1D\t\t\t: " << limits.maxImageDimensions1D << std::endl;
            ss << "\tMax Image Dimensions 2D\t\t\t: " << limits.maxImageDimensions2D << std::endl;
            ss << "\tMax Image Dimensions 3D\t\t\t: " << limits.maxImageDimensions3D << std::endl;
            ss << "\tMax Image Dimensions Cube\t\t: " << limits.maxImageDimensionsCube << std::endl;
            ss << "\tMax Image Array Layers\t\t\t: " << limits.maxImageArrayLayers << std::endl;
            ss << "\tMax Uniform Buffer Range\t\t: " << limits.maxUniformBufferRange << std::endl;
            ss << "\tMax Storage Buffer Range\t\t: " << limits.maxStorageBufferRange << std::endl;
            ss << "\tMax Push Constants Size\t\t\t: " << limits.maxPushConstantsSize << std::endl;
            ss << "\tMax Compute Shared Memory Size\t\t: " << limits.maxComputeSharedMemorySize << std::endl;
            ss << "\tMax Compute Work Group Count\t\t: (" << limits.maxComputeWorkGroupCount[0] << ", " << limits.maxComputeWorkGroupCount[1] << ", " << limits.maxComputeWorkGroupCount[2] << ")" << std::endl;
            ss << "\tMax Compute Work Group Invocations\t: " << limits.maxComputeWorkGroupInvocations << std::endl;
            ss << "\tMax Compute Work Group Size\t\t: (" << limits.maxComputeWorkGroupSize[0] << ", " << limits.maxComputeWorkGroupSize[1] << ", " << limits.maxComputeWorkGroupSize[2] << ")" << std::endl;
            ss << "\tMax Draw Indexed Index Value\t\t: " << limits.maxDrawIndexedIndexValue << std::endl;
            ss << "\tMax Viewports\t\t\t\t: " << limits.maxViewports << std::endl;
            ss << "\tMax Viewport Dimensions\t\t\t: " << limits.maxViewportDimensions[0] << " " << limits.maxViewportDimensions[1] << std::endl;
            ss << "\tLine Width Range\t\t\t: (" << limits.lineWidthRange[0] << ", " << limits.lineWidthRange[1] << ")" << std::endl;
            ss << "\tPoint Size Range\t\t\t: (" << limits.pointSizeRange[0] << ", " << limits.pointSizeRange[1] << ")" << std::endl;
            ss << std::endl;

            /* Device Features */
            ss << "Features:- " << std::endl;
            ss << "\tImage Cube Array\t\t: " << ( features.imageCubeArray ? "Available" : "Not Available" ) << std::endl;
            ss << "\tGeometry Shader\t\t\t: " << ( features.geometryShader ? "Available" : "Not Available" ) << std::endl;
            ss << "\tTessellation Shader\t\t: " << ( features.tessellationShader ? "Available" : "Not Available" ) << std::endl;
            ss << "\tLogic Op\t\t\t: " << ( features.logicOp ? "Available" : "Not Available" ) << std::endl;
            ss << "\tMulti Draw Indirect\t\t: " << ( features.multiDrawIndirect ? "Available" : "Not Available" ) << std::endl;
            ss << "\tWide Lines\t\t\t: " << ( features.wideLines ? "Available" : "Not Available" ) << std::endl;
            ss << "\tLarge Points\t\t\t: " << ( features.largePoints ? "Available" : "Not Available" ) << std::endl;
            ss << "\tTexture Compression ETC2\t: " << ( features.textureCompressionETC2 ? "Available" : "Not Available" ) << std::endl;
            ss << "\tTexture Compression ASTC LDR\t: " << ( features.textureCompressionASTC_LDR ? "Available" : "Not Available" ) << std::endl;
            ss << "\tTexture Compression BC\t\t: " << ( features.textureCompressionBC ? "Available" : "Not Available" ) << std::endl;
            ss << std::endl;

            /* Device Queue Family Properties */
            ss << "Queue Family Properties:- " << std::endl;
            ss << "\tQueue Family Count\t: " << queueFamilyProperties.size() << std::endl;
            ss << "\tQueue Families:- " << std::endl;
            for (auto& queueFamilyProperty : queueFamilyProperties)
            {
                ss << "\t\tIndex\t\t: " << queueFamilyProperty.index << std::endl;
                ss << "\t\tQueue Count\t: " << queueFamilyProperty.queueCount << std::endl;
                ss << "\t\tAvailable Queue Types:- " << std::endl;
                if (queueFamilyProperty.graphics)
                    ss << "\t\t\tGraphics" << std::endl;
                if (queueFamilyProperty.compute)
                    ss << "\t\t\tCompute" << std::endl;
                if (queueFamilyProperty.transfer)
                    ss << "\t\t\tTransfer" << std::endl;
                if (queueFamilyProperty.sparseBinding)
                    ss << "\t\t\tSparse Binding" << std::endl;               
            }
            ss << std::endl;

            return ss.str();
        }

        uint32_t PhysicalDevice::GetGraphicsQueueIndex()
        {
            for (auto& q : queueFamilyProperties)
            {
                if (q.graphics)
                {
                    return q.index;
                }
            }
            return -1;
        }

        uint32_t PhysicalDevice::GetComputeQueueIndex()
        {
            for (auto& q : queueFamilyProperties)
            {
                if (q.compute)
                {
                    return q.index;
                }
            }
            return -1;
        }

        uint32_t PhysicalDevice::GetPresentQueueIndex(VkSurfaceKHR surface)
        {
            uint32_t index = -1;
            VkBool32 supported = false;
            for (auto& queueFamilyProperty : queueFamilyProperties)
            {
                supported = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(handle, queueFamilyProperty.index, surface, &supported);
                if (supported)
                    return queueFamilyProperty.index;
            }
            return index;
        }

        PhysicalDeviceQueueFamilyProperties PhysicalDevice::GetQueue(uint32_t index)
        {
            for (auto& queue : queueFamilyProperties)
            {
                if (queue.index == index)
                    return queue;
            }
            return PhysicalDeviceQueueFamilyProperties();
        }

        bool PhysicalDevice::IsSurfaceSupported(VkSurfaceKHR surface, uint32_t queueFamilyIndex)
        {
            VkBool32 supported;
            vkGetPhysicalDeviceSurfaceSupportKHR(handle, queueFamilyIndex, surface, &supported);
            return supported == VK_TRUE;
        }


        std::string to_string(PhysicalDeviceType type)
        {
            switch (type)
            {
            case PhysicalDeviceType::PhysicalDeviceType_Unknown:
                return "Unknown";
            case PhysicalDeviceType::PhysicalDeviceType_IntegratedGPU:
                return "Integrated GPU";
            case PhysicalDeviceType::PhysicalDeviceType_DiscreteGPU:
                return "Discrete GPU";
            case PhysicalDeviceType::PhysicalDeviceType_VirtualGPU:
                return "Virtual GPU";
            case PhysicalDeviceType::PhysicalDeviceType_CPU:
                return "CPU";
            default:
                return "Unknown";
            }
        }

    }
}
