#pragma once

#include "Base/Base.h"
#include "Generators/DEM/DEMTileTypes.h"

#include <memory>
#include <string>
#include <unordered_map>

TF3D_FWD_DEC_CLASS(Texture2D, tf3d::base)

namespace tf3d::generators::dem
{

    class TileCache
    {
    public:
        explicit TileCache(std::string cacheDirectory);

        std::string PathFor(const TileKey &key, TileAsset asset) const;
        bool HasDiskEntry(const TileKey &key, TileAsset asset) const;
        void RemoveDiskEntry(const TileKey &key, TileAsset asset) const;

        std::shared_ptr<base::Texture2D> FindLoaded(const TileKey &key, TileAsset asset) const;
        void StoreLoaded(const TileKey &key, TileAsset asset, std::shared_ptr<base::Texture2D> texture);

        inline const std::string &FileFormat(TileAsset asset) const
        {
            return asset == TileAsset::Satellite ? m_SatelliteFileFormat : m_ElevationFileFormat;
        }

    private:
        std::string m_ElevationFileFormat;
        std::string m_SatelliteFileFormat;
        std::unordered_map<TileKey, std::shared_ptr<base::Texture2D>, TileKeyHash> m_LoadedElevationTiles;
        std::unordered_map<TileKey, std::shared_ptr<base::Texture2D>, TileKeyHash> m_LoadedSatelliteTiles;
    };

} // namespace tf3d::generators::dem
