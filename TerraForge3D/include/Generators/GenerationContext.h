#pragma once

#include <cstdint>

namespace tf3d::generators
{

    class GeneratorTexture;

    struct GenerationContext {
        GeneratorTexture *seedTexture = nullptr;
        int32_t tileResolution        = 0;
        int32_t gpuWorkgroupSize      = 1;
        float tileSize                = 1.0f;
    };

} // namespace tf3d::generators
