#pragma once
#include "Base/Base.hpp"

namespace TerraForge3D
{
    enum TerrainGeneratorDevice
    {
        TerrainGeneratorDevice_CPU = 0,
        TerrainGeneratorDevice_OpenCL,
        TerrainGeneratorDevice_VulkanCompute
    };

    struct TerrainPointData
    {
        glm::vec4 a;
        glm::vec4 b;
        glm::vec4 c;
        glm::vec4 d;
    };

    struct TerrainGeneratorState
    {
        float offset[4];
        float size[4];
        TerrainGeneratorDevice device;
    };

}
