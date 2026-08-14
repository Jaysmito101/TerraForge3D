#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <httplib/httplib.h>
#include <utility>

#include "Base/CircuitBreaker.h"
#include "Generators/DEM/DEMTileDownloader.h"

#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

namespace tf3d::generators::dem
{

    struct TileDownloader::SharedState {
        mutable std::mutex mutex;
        std::condition_variable condition;
        std::deque<QueuedRequest> queue;
        std::unordered_set<TileKey, TileKeyHash> pending;
        std::string apiKey;
        std::string elevationCacheFileFormat;
        std::string satelliteCacheFileFormat;
        std::string apiHostURL                                = "https://api.maptiler.com";
        std::string elevationApiPathURLFormat                 = "/tiles/terrain-rgb-v2/{0}/{1}/{2}.webp?key={3}";
        std::string satelliteApiPathURLFormat                 = "/tiles/satellite-v2/{0}/{1}/{2}.jpg?key={3}";
        tf3d::data::ApplicationState *appState                = nullptr;
        bool alive                                            = true;
        bool allowRequests                                    = true;
        int32_t requestsThisUpdate                            = 0;
        uint64_t cancellationGeneration                       = 0;
        std::chrono::steady_clock::time_point nextRequestTime = std::chrono::steady_clock::time_point::min();
        base::CircuitBreaker circuit{TileDownloader::kCircuitFailureThreshold,
                                     std::chrono::seconds(TileDownloader::kCircuitOpenSeconds)};
    };

    TileDownloader::TileDownloader(tf3d::data::ApplicationState *appState,
                                   std::string elevationCacheFileFormat,
                                   std::string satelliteCacheFileFormat)
        : m_AppState(appState),
          m_State(std::make_shared<SharedState>())
    {
        if (m_AppState == nullptr) {
            return;
        }

        m_State->elevationCacheFileFormat = std::move(elevationCacheFileFormat);
        m_State->satelliteCacheFileFormat = std::move(satelliteCacheFileFormat);
        m_State->appState                 = appState;

        if (m_AppState->configManager != nullptr) {
            m_AppState->configManager->GetString("apiKeys", "maptilerCloud", m_State->apiKey);
        }

        m_WorkerThread = std::thread(&TileDownloader::Run, this);
    }

    TileDownloader::~TileDownloader()
    {
        {
            std::lock_guard lock(m_State->mutex);
            m_State->alive = false;
            m_State->queue.clear();
        }
        m_State->condition.notify_one();

        if (m_WorkerThread.joinable()) {
            m_WorkerThread.join();
        }
    }

    void TileDownloader::BeginUpdate(bool allowRequests)
    {
        {
            std::lock_guard lock(m_State->mutex);
            m_State->allowRequests      = allowRequests;
            m_State->requestsThisUpdate = 0;
            if (!allowRequests) {
                ++m_State->cancellationGeneration;
                for (const auto &request : m_State->queue) {
                    m_State->pending.erase(request.key);
                }
                m_State->queue.clear();
                m_State->circuit.AbortRequest();
            }
        }
        m_State->condition.notify_one();
    }

    TileRequestResult TileDownloader::Request(const TileKey &key)
    {
        if (!key.IsValid() || m_AppState == nullptr) {
            return TileRequestResult::Disabled;
        }

        const auto now = std::chrono::steady_clock::now();
        {
            std::lock_guard lock(m_State->mutex);
            if (m_State->pending.contains(key)) {
                return TileRequestResult::AlreadyPending;
            }

            if (m_State->apiKey.empty()) {
                return TileRequestResult::Disabled;
            }

            if (!m_State->allowRequests ||
                static_cast<int32_t>(m_State->pending.size()) >= kMaxPendingRequests ||
                m_State->requestsThisUpdate >= kMaxRequestsPerUpdate ||
                now < m_State->nextRequestTime) {
                return TileRequestResult::RateLimited;
            }

            const auto elevationCachePath = FormatPath(m_State->elevationCacheFileFormat, key);
            const auto satelliteCachePath = FormatPath(m_State->satelliteCacheFileFormat, key);
            const bool needsElevation     = !PathExist(elevationCachePath);
            const bool needsSatellite     = !PathExist(satelliteCachePath);
            if (!needsElevation && !needsSatellite) {
                return TileRequestResult::AlreadyCached;
            }

            if (!m_State->circuit.AllowRequest()) {
                return TileRequestResult::CircuitOpen;
            }

            m_State->pending.insert(key);
            m_State->requestsThisUpdate += 1;
            m_State->nextRequestTime = now + std::chrono::milliseconds(kRequestIntervalMilliseconds);

            QueuedRequest request{};
            request.key        = key;
            request.generation = m_State->cancellationGeneration;
            request.apiHost    = m_State->apiHostURL;
            request.assets.reserve(2);
            if (needsElevation) {
                const auto urlPath = fmt::vformat(m_State->elevationApiPathURLFormat,
                                                  fmt::make_format_args(key.zoom, key.x, key.y, m_State->apiKey));
                request.assets.push_back({TileAsset::Elevation,
                                          elevationCachePath,
                                          elevationCachePath + ".part",
                                          urlPath});
            }

            if (needsSatellite) {
                const auto urlPath = fmt::vformat(m_State->satelliteApiPathURLFormat,
                                                  fmt::make_format_args(key.zoom, key.x, key.y, m_State->apiKey));
                request.assets.push_back({TileAsset::Satellite,
                                          satelliteCachePath,
                                          satelliteCachePath + ".part",
                                          urlPath});
            }

            m_State->queue.push_back(std::move(request));
        }

        m_State->condition.notify_one();

        return TileRequestResult::Scheduled;
    }

    bool TileDownloader::IsCancelled(const std::shared_ptr<SharedState> &state, uint64_t generation)
    {
        std::lock_guard lock(state->mutex);
        return !state->alive || state->cancellationGeneration != generation;
    }

    TileDownloader::DownloadResult TileDownloader::Download(const std::shared_ptr<SharedState> &state,
                                                            const QueuedRequest &request)
    {
        for (const auto &asset : request.assets) {
            if (IsCancelled(state, request.generation)) {
                return DownloadResult::Cancelled;
            }

            std::remove(asset.temporaryPath.c_str());
            std::ofstream output(asset.temporaryPath, std::ios::binary | std::ios::trunc);
            if (!output.is_open())
                return DownloadResult::Failed;

            httplib::Client client(request.apiHost);
            client.set_connection_timeout(std::chrono::seconds(5));
            client.set_read_timeout(std::chrono::seconds(5));

            bool cancelled    = false;
            const auto result = client.Get(asset.urlPath, [&](const char *data, size_t dataLength) {
                if (IsCancelled(state, request.generation)) {
                    cancelled = true;
                    return false;
                }

                output.write(data, static_cast<std::streamsize>(dataLength));
                return output.good();
            });
            output.close();

            if (cancelled || IsCancelled(state, request.generation)) {
                std::remove(asset.temporaryPath.c_str());
                return DownloadResult::Cancelled;
            }

            const char *assetName = asset.asset == TileAsset::Satellite ? "satellite" : "elevation";
            if (!result || result->status != 200) {
                std::remove(asset.temporaryPath.c_str());
                if (result) {
                    TF3D_LOG_WARN("DEM {} tile download failed with HTTP status {} for ({}, {}, {})",
                                  assetName,
                                  result->status,
                                  request.key.x,
                                  request.key.y,
                                  request.key.zoom);
                } else {
                    TF3D_LOG_WARN("DEM {} tile download failed for ({}, {}, {})",
                                  assetName,
                                  request.key.x,
                                  request.key.y,
                                  request.key.zoom);
                }
                return DownloadResult::Failed;
            }

            std::error_code error;
            if (!PathExist(asset.temporaryPath) ||
                std::filesystem::file_size(asset.temporaryPath, error) == 0 || error) {
                std::remove(asset.temporaryPath.c_str());
                return DownloadResult::Failed;
            }

            if (IsCancelled(state, request.generation)) {
                std::remove(asset.temporaryPath.c_str());
                return DownloadResult::Cancelled;
            }

            std::remove(asset.cachePath.c_str());
            if (std::rename(asset.temporaryPath.c_str(), asset.cachePath.c_str()) != 0)
                return DownloadResult::Failed;
        }

        return DownloadResult::Succeeded;
    }

    void TileDownloader::Run()
    {
        const auto state = m_State;
        while (true) {
            QueuedRequest request{};
            {
                std::unique_lock lock(state->mutex);
                state->condition.wait(lock, [&state] {
                    return !state->alive || !state->queue.empty();
                });

                if (!state->alive) {
                    return;
                }

                request = std::move(state->queue.front());
                state->queue.pop_front();
            }

            const auto result                      = Download(state, request);
            tf3d::data::ApplicationState *appState = nullptr;
            {
                std::lock_guard lock(state->mutex);
                if (!state->alive)
                    return;

                state->pending.erase(request.key);
                if (result == DownloadResult::Succeeded)
                    state->circuit.RecordSuccess();
                else if (result == DownloadResult::Failed)
                    state->circuit.RecordFailure();
                else
                    state->circuit.AbortRequest();
                appState = state->appState;
            }

            if ((result == DownloadResult::Succeeded || result == DownloadResult::Failed) && appState != nullptr)
                appState->generationDirtyManager.MarkForce(GenerationDirtyCause::External);
        }
    }

    void TileDownloader::CancelPendingRequests()
    {
        {
            std::lock_guard lock(m_State->mutex);
            ++m_State->cancellationGeneration;
            for (const auto &request : m_State->queue) {
                m_State->pending.erase(request.key);
            }
            m_State->queue.clear();
            m_State->circuit.AbortRequest();
        }
        m_State->condition.notify_one();
    }

    void TileDownloader::SetApiKey(std::string apiKey)
    {
        {
            std::lock_guard lock(m_State->mutex);
            m_State->apiKey = std::move(apiKey);
            m_State->circuit.Reset();
        }
        CancelPendingRequests();

        if (m_AppState == nullptr || m_AppState->configManager == nullptr) {
            TF3D_LOG_WARN("Could not save the MapTiler key because the user configuration store is unavailable.");
            return;
        }

        if (!m_AppState->configManager->SetString("apiKeys", "maptilerCloud", GetApiKey()))
            TF3D_LOG_WARN("Could not save the MapTiler key in the user configuration store.");
    }

    std::string TileDownloader::GetApiKey() const
    {
        std::lock_guard lock(m_State->mutex);
        return m_State->apiKey;
    }

    bool TileDownloader::HasApiKey() const
    {
        std::lock_guard lock(m_State->mutex);
        return !m_State->apiKey.empty();
    }

    bool TileDownloader::IsPending(const TileKey &key) const
    {
        std::lock_guard lock(m_State->mutex);
        return m_State->pending.contains(key);
    }

    std::size_t TileDownloader::PendingCount() const
    {
        std::lock_guard lock(m_State->mutex);
        return m_State->pending.size();
    }

    bool TileDownloader::IsCircuitOpen() const
    {
        return m_State->circuit.IsOpen();
    }

    std::string TileDownloader::FormatPath(const std::string &fileFormat, const TileKey &key)
    {
        return fmt::vformat(fileFormat, fmt::make_format_args(key.x, key.y, key.zoom));
    }

} // namespace tf3d::generators::dem
