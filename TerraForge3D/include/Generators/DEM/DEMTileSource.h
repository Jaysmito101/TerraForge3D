#pragma once

#include "Base/Base.h"
#include "Generators/DEM/DEMTileCache.h"
#include "Generators/DEM/DEMTileDownloader.h"

#include <cstddef>
#include <memory>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)
TF3D_FWD_DEC_CLASS(TextureLoader, tf3d::base)

namespace tf3d::generators::dem
{

    class TileSource
    {
    public:
        explicit TileSource(tf3d::data::ApplicationState *appState);

        void BeginUpdate(bool allowRequests);
        void CancelPendingRequests();

        std::shared_ptr<base::Texture2D> Request(const TileKey &key);
        std::shared_ptr<base::Texture2D> RequestSatellite(const TileKey &key);
        std::shared_ptr<base::Texture2D> FindLoaded(const TileKey &key) const;
        std::shared_ptr<base::Texture2D> FindLoadedSatellite(const TileKey &key) const;
        std::shared_ptr<base::Texture2D> FindBestAvailable(const TileKey &requestedKey, TileKey &resolvedKey);
        bool IsPending(const TileKey &key) const;
        bool IsTexturePending(const TileKey &key) const;
        bool IsCircuitOpen() const;

        void SetApiKey(std::string apiKey);
        std::string GetApiKey() const;
        bool HasApiKey() const;

        std::size_t PendingCount() const;

        inline int32_t MaxPendingRequests() const
        {
            return TileDownloader::kMaxPendingRequests;
        }

        inline int32_t MaxRequestsPerUpdate() const
        {
            return TileDownloader::kMaxRequestsPerUpdate;
        }

        inline int32_t RequestIntervalMilliseconds() const
        {
            return TileDownloader::kRequestIntervalMilliseconds;
        }

    private:
        bool QueueTextureLoad(const TileKey &key, TileAsset asset);
        static std::shared_ptr<base::Texture2D> DecodeElevation(const std::string &path);

        tf3d::data::ApplicationState *m_AppState = nullptr;
        base::TextureLoader *m_TextureLoader     = nullptr;
        TileCache m_Cache;
        std::shared_ptr<base::Texture2D> m_LoadingTexture;
        std::shared_ptr<base::Texture2D> m_UnavailableTexture;
        TileDownloader m_Downloader;
    };

} // namespace tf3d::generators::dem
