#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace tf3d::generators::dem
{

    inline constexpr uint32_t kMaxTileZoom = 12;

    enum class TileAsset {
        Elevation,
        Satellite
    };

    struct TileKey {
        uint32_t x    = 0;
        uint32_t y    = 0;
        uint32_t zoom = 0;

        constexpr bool operator==(const TileKey &) const = default;

        constexpr bool IsValid() const noexcept
        {
            if (zoom > kMaxTileZoom) {
                return false;
            }

            const uint32_t tileCount = 1u << zoom;
            return x < tileCount && y < tileCount;
        }
    };

    struct TileKeyHash {
        std::size_t operator()(const TileKey &key) const noexcept
        {
            const auto hashValue = std::hash<uint32_t>{};
            return ((hashValue(key.x) ^ (hashValue(key.y) << 1)) >> 1) ^
                   (hashValue(key.zoom) << 1);
        }
    };

} // namespace tf3d::generators::dem
