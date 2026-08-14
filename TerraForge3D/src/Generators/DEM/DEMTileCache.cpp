#include "Generators/DEM/DEMTileCache.h"

#include "Utils/Utils.h"

#include <filesystem>
#include <fmt/format.h>
#include <utility>

namespace tf3d::generators::dem
{

    TileCache::TileCache(std::string cacheDirectory)
    {
        const auto elevationDirectory = std::filesystem::path(std::move(cacheDirectory));
        const auto satelliteDirectory = elevationDirectory.parent_path() / "satellite";
        std::error_code error;
        std::filesystem::create_directories(elevationDirectory, error);
        error.clear();
        std::filesystem::create_directories(satelliteDirectory, error);
        m_ElevationFileFormat = (elevationDirectory / "{}_{}_{}.webp").string();
        m_SatelliteFileFormat = (satelliteDirectory / "{}_{}_{}.jpg").string();
    }

    std::string TileCache::PathFor(const TileKey &key, TileAsset asset) const
    {
        return fmt::vformat(FileFormat(asset), fmt::make_format_args(key.x, key.y, key.zoom));
    }

    bool TileCache::HasDiskEntry(const TileKey &key, TileAsset asset) const
    {
        return PathExist(PathFor(key, asset));
    }

    void TileCache::RemoveDiskEntry(const TileKey &key, TileAsset asset) const
    {
        std::remove(PathFor(key, asset).c_str());
    }

    std::shared_ptr<base::Texture2D> TileCache::FindLoaded(const TileKey &key, TileAsset asset) const
    {
        const auto &loadedTiles = asset == TileAsset::Satellite ? m_LoadedSatelliteTiles : m_LoadedElevationTiles;
        const auto iterator     = loadedTiles.find(key);
        return iterator != loadedTiles.end() ? iterator->second : nullptr;
    }

    void TileCache::StoreLoaded(const TileKey &key, TileAsset asset, std::shared_ptr<base::Texture2D> texture)
    {
        if (texture == nullptr)
            return;

        auto &loadedTiles = asset == TileAsset::Satellite ? m_LoadedSatelliteTiles : m_LoadedElevationTiles;
        loadedTiles[key]  = std::move(texture);
    }

} // namespace tf3d::generators::dem
