#include "Generators/DEM/DEMTileSource.h"

#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

#include <utility>

namespace tf3d::generators::dem
{

    TileSource::TileSource(tf3d::data::ApplicationState *appState)
        : m_Cache(appState != nullptr
                      ? appState->constants.cacheDir + PATH_SEPARATOR "dem_data" PATH_SEPARATOR "terrain_rgb"
                      : std::string(".")),
          m_Loader(appState != nullptr ? appState->constants.texturesDir : std::string()),
          m_Downloader(appState,
                       m_Cache.FileFormat(TileAsset::Elevation),
                       m_Cache.FileFormat(TileAsset::Satellite))
    {
    }

    void TileSource::BeginUpdate(bool allowRequests)
    {
        m_Downloader.BeginUpdate(allowRequests);
    }

    void TileSource::CancelPendingRequests()
    {
        m_Downloader.CancelPendingRequests();
    }

    std::shared_ptr<base::Texture2D> TileSource::Request(const TileKey &key)
    {
        if (!key.IsValid())
            return m_Loader.UnavailableTexture();

        if (const auto loaded = m_Cache.FindLoaded(key, TileAsset::Elevation); loaded != nullptr) {
            if (!m_Cache.HasDiskEntry(key, TileAsset::Satellite))
                m_Downloader.Request(key);
            return loaded;
        }

        if (!m_Cache.HasDiskEntry(key, TileAsset::Elevation)) {
            const auto requestResult = m_Downloader.Request(key);
            if (requestResult == TileRequestResult::Scheduled ||
                requestResult == TileRequestResult::AlreadyPending ||
                requestResult == TileRequestResult::RateLimited) {
                return m_Loader.LoadingTexture();
            }
            if (!m_Cache.HasDiskEntry(key, TileAsset::Elevation))
                return m_Loader.UnavailableTexture();
        }

        if (!m_Cache.HasDiskEntry(key, TileAsset::Satellite)) {
            m_Downloader.Request(key);
        }

        const auto loadResult = m_Loader.Load(m_Cache.PathFor(key, TileAsset::Elevation));
        if (loadResult.status == TileLoadStatus::Loaded) {
            m_Cache.StoreLoaded(key, TileAsset::Elevation, loadResult.texture);
            return loadResult.texture;
        }

        m_Cache.RemoveDiskEntry(key, TileAsset::Elevation);
        return m_Loader.UnavailableTexture();
    }

    std::shared_ptr<base::Texture2D> TileSource::RequestSatellite(const TileKey &key)
    {
        if (!key.IsValid()) {
            return m_Loader.UnavailableTexture();
        }

        if (const auto loaded = m_Cache.FindLoaded(key, TileAsset::Satellite); loaded != nullptr) {
            return loaded;
        }

        if (!m_Cache.HasDiskEntry(key, TileAsset::Satellite)) {
            const auto requestResult = m_Downloader.Request(key);
            if (requestResult == TileRequestResult::Scheduled ||
                requestResult == TileRequestResult::AlreadyPending ||
                requestResult == TileRequestResult::RateLimited) {
                return m_Loader.LoadingTexture();
            }
            if (!m_Cache.HasDiskEntry(key, TileAsset::Satellite))
                return m_Loader.UnavailableTexture();
        }

        const auto loadResult = m_Loader.LoadSatellite(m_Cache.PathFor(key, TileAsset::Satellite));
        if (loadResult.status == TileLoadStatus::Loaded) {
            m_Cache.StoreLoaded(key, TileAsset::Satellite, loadResult.texture);
            return loadResult.texture;
        }

        m_Cache.RemoveDiskEntry(key, TileAsset::Satellite);
        return m_Loader.UnavailableTexture();
    }

    std::shared_ptr<base::Texture2D> TileSource::FindLoaded(const TileKey &key) const
    {
        return m_Cache.FindLoaded(key, TileAsset::Elevation);
    }

    std::shared_ptr<base::Texture2D> TileSource::FindLoadedSatellite(const TileKey &key) const
    {
        return m_Cache.FindLoaded(key, TileAsset::Satellite);
    }

    bool TileSource::IsPending(const TileKey &key) const
    {
        return m_Downloader.IsPending(key);
    }

    bool TileSource::IsCircuitOpen() const
    {
        return m_Downloader.IsCircuitOpen();
    }

    std::shared_ptr<base::Texture2D> TileSource::FindBestAvailable(const TileKey &requestedKey,
                                                                   TileKey &resolvedKey)
    {
        if (requestedKey.zoom == 0)
            return nullptr;

        bool requestedNearestFallback = false;
        for (int32_t fallbackZoom = static_cast<int32_t>(requestedKey.zoom) - 1;
             fallbackZoom >= 0;
             --fallbackZoom) {
            const uint32_t shift = requestedKey.zoom - static_cast<uint32_t>(fallbackZoom);
            const TileKey candidate{requestedKey.x >> shift,
                                    requestedKey.y >> shift,
                                    static_cast<uint32_t>(fallbackZoom)};

            if (const auto loaded = m_Cache.FindLoaded(candidate, TileAsset::Elevation); loaded != nullptr) {
                resolvedKey = candidate;
                return loaded;
            }

            if (!m_Cache.HasDiskEntry(candidate, TileAsset::Elevation)) {
                if (!requestedNearestFallback) {
                    Request(candidate);
                    requestedNearestFallback = true;
                }
                continue;
            }

            Request(candidate);
            if (const auto cached = m_Cache.FindLoaded(candidate, TileAsset::Elevation); cached != nullptr) {
                resolvedKey = candidate;
                return cached;
            }
        }

        return nullptr;
    }

    void TileSource::SetApiKey(std::string apiKey)
    {
        m_Downloader.SetApiKey(std::move(apiKey));
    }

    std::string TileSource::GetApiKey() const
    {
        return m_Downloader.GetApiKey();
    }

    bool TileSource::HasApiKey() const
    {
        return m_Downloader.HasApiKey();
    }

    std::size_t TileSource::PendingCount() const
    {
        return m_Downloader.PendingCount();
    }

} // namespace tf3d::generators::dem
