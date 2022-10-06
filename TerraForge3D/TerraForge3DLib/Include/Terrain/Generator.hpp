#pragma once
#include "Terrain/Data.hpp"

namespace TerraForge3D
{
    class TerrainGenerator
    {
    public:
        TerrainGenerator(std::string name);
        virtual ~TerrainGenerator();

        virtual bool IsDeviceSupported(const TerrainGeneratorDevice device) const = 0;
        virtual bool Generate(const TerrainGeneratorState* state) = 0;
        virtual bool ShowSettings() = 0;

    protected:
        std::string name = "Generator";
    };
}