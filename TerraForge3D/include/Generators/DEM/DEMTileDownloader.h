#pragma once

#include "Base/Base.h"
#include "Generators/DEM/DEMTileTypes.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)
TF3D_FWD_DEC_CLASS(TileSource, tf3d::generators::dem)

namespace tf3d::generators::dem
{

    enum class TileRequestResult {
        Scheduled,
        AlreadyPending,
        AlreadyCached,
        RateLimited,
        CircuitOpen,
        Disabled,
    };

    class TileDownloader
    {
    public:
        static constexpr int32_t kMaxPendingRequests          = 8;
        static constexpr int32_t kMaxRequestsPerUpdate        = 4;
        static constexpr int32_t kRequestIntervalMilliseconds = 300;
        static constexpr int32_t kCircuitFailureThreshold     = 3;
        static constexpr int32_t kCircuitOpenSeconds          = 30;

        TileDownloader(tf3d::data::ApplicationState *appState,
                       std::string elevationCacheFileFormat,
                       std::string satelliteCacheFileFormat);
        ~TileDownloader();

        void BeginUpdate(bool allowRequests);
        TileRequestResult Request(const TileKey &key);

        void SetApiKey(std::string apiKey);
        std::string GetApiKey() const;
        bool HasApiKey() const;

        size_t PendingCount() const;

    private:
        friend class TileSource;

        enum class DownloadResult {
            Succeeded,
            Cancelled,
            Failed
        };

        struct QueuedAsset {
            TileAsset asset = TileAsset::Elevation;
            std::string cachePath;
            std::string temporaryPath;
            std::string urlPath;
        };

        struct QueuedRequest {
            TileKey key;
            uint64_t generation = 0;
            std::string apiHost;
            std::vector<QueuedAsset> assets;
        };

        struct SharedState;

        static std::string FormatPath(const std::string &fileFormat, const TileKey &key);
        static DownloadResult Download(const std::shared_ptr<SharedState> &state,
                                       const QueuedRequest &request);
        static bool IsCancelled(const std::shared_ptr<SharedState> &state, uint64_t generation);

        void Run();
        void CancelPendingRequests();
        bool IsPending(const TileKey &key) const;
        bool IsCircuitOpen() const;

        tf3d::data::ApplicationState *m_AppState = nullptr;
        std::shared_ptr<SharedState> m_State;
        std::thread m_WorkerThread;
    };

} // namespace tf3d::generators::dem
