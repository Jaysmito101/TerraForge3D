#pragma once

#include "Generators/GenerationDirtyManager.h"

#include <cstdint>

namespace tf3d::generators
{

    class GeneratorTexture;

    struct GenerationContext {
        GeneratorTexture *seedTexture              = nullptr;
        int32_t tileResolution                     = 0;
        int32_t gpuWorkgroupSize                   = 1;
        float tileSize                             = 1.0f;
        const GenerationDirtyManager *dirtyManager = nullptr;
        uint64_t inputRevision                     = 0;

        inline bool IsCurrent() const
        {
            return dirtyManager == nullptr || dirtyManager->IsRevisionCurrent(inputRevision);
        }
    };

} // namespace tf3d::generators
