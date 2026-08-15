#include "Generators/DEM/DEMTileSource.h"

#include "Base/Texture2D.h"
#include "Base/TextureLoader.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"
#include "webp/decode.h"

#include <filesystem>
#include <utility>

namespace tf3d::generators::dem
{

    TileSource::TileSource(tf3d::data::ApplicationState *appState)
        : m_AppState(appState),
          m_TextureLoader(appState != nullptr ? appState->textureLoader.get() : nullptr),
          m_Cache(appState != nullptr
                      ? appState->constants.cacheDir + PATH_SEPARATOR "dem_data" PATH_SEPARATOR "terrain_rgb"
                      : std::string(".")),
          m_LoadingTexture(std::make_shared<base::Texture2D>(
              (std::filesystem::path(appState != nullptr ? appState->constants.texturesDir : std::string()) /
               "loading.png")
                  .string(),
              false)),
          m_UnavailableTexture(std::make_shared<base::Texture2D>(
              (std::filesystem::path(appState != nullptr ? appState->constants.texturesDir : std::string()) /
               "black.jpg")
                  .string(),
              false)),
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
        if (!key.IsValid()) {
            return m_UnavailableTexture;
        }

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
                return m_LoadingTexture;
            }
            if (!m_Cache.HasDiskEntry(key, TileAsset::Elevation)) {
                return m_UnavailableTexture;
            }
        }

        if (!m_Cache.HasDiskEntry(key, TileAsset::Satellite)) {
            m_Downloader.Request(key);
        }

        if (QueueTextureLoad(key, TileAsset::Elevation)) {
            return m_LoadingTexture;
        }
        return m_UnavailableTexture;
    }

    std::shared_ptr<base::Texture2D> TileSource::RequestSatellite(const TileKey &key)
    {
        if (!key.IsValid()) {
            return m_UnavailableTexture;
        }

        if (const auto loaded = m_Cache.FindLoaded(key, TileAsset::Satellite); loaded != nullptr) {
            return loaded;
        }

        if (!m_Cache.HasDiskEntry(key, TileAsset::Satellite)) {
            const auto requestResult = m_Downloader.Request(key);
            if (requestResult == TileRequestResult::Scheduled ||
                requestResult == TileRequestResult::AlreadyPending ||
                requestResult == TileRequestResult::RateLimited) {
                return m_LoadingTexture;
            }
            if (!m_Cache.HasDiskEntry(key, TileAsset::Satellite)) {
                return m_UnavailableTexture;
            }
        }

        if (QueueTextureLoad(key, TileAsset::Satellite)) {
            return m_LoadingTexture;
        }
        return m_UnavailableTexture;
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

    bool TileSource::IsTexturePending(const TileKey &key) const
    {
        return key.IsValid() && m_TextureLoader != nullptr &&
               m_TextureLoader->IsPending(m_Cache.PathFor(key, TileAsset::Elevation));
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

    bool TileSource::QueueTextureLoad(const TileKey &key, TileAsset asset)
    {
        if (m_TextureLoader == nullptr || !m_TextureLoader->HasContext()) {
            return false;
        }

        const std::string path = m_Cache.PathFor(key, asset);
        const auto priority    = asset == TileAsset::Elevation
                                     ? base::TextureLoadPriority::High
                                     : base::TextureLoadPriority::Low;
        auto completionHandler = [this, key, asset](base::TextureLoadResult result) {
            if (result.Succeeded()) {
                m_Cache.StoreLoaded(key, asset, std::move(result.texture));
                if (m_AppState != nullptr) {
                    m_AppState->generationDirtyManager.MarkForce(GenerationDirtyCause::External);
                }
            } else {
                m_Cache.RemoveDiskEntry(key, asset);
            }
        };

        if (asset == TileAsset::Elevation) {
            m_TextureLoader->Request(path,
                                     priority,
                                     base::TextureLoader::DecodeFunction(&TileSource::DecodeElevation),
                                     std::move(completionHandler));
        } else {
            m_TextureLoader->Request(path,
                                     priority,
                                     std::move(completionHandler));
        }
        return true;
    }

    std::shared_ptr<base::Texture2D> TileSource::DecodeElevation(const std::string &path)
    {
        int width = 1;
        int height = 1;
        int size = 0;
        uint8_t *data = reinterpret_cast<uint8_t *>(ReadBinaryFile(path, &size));
        if (data == nullptr || size <= 0) {
            delete[] data;
            return nullptr;
        }

        auto rgbaData = WebPDecodeRGBA(data, size, &width, &height);
        if (rgbaData == nullptr || width <= 0 || height <= 0) {
            delete[] data;
            if (rgbaData != nullptr) {
                free(rgbaData);
            }
            return nullptr;
        }

        auto texture = std::make_shared<base::Texture2D>(width, height);
        texture->SetData(rgbaData, 0, true);
        delete[] data;
        free(rgbaData);
        return texture;
    }

} // namespace tf3d::generators::dem
