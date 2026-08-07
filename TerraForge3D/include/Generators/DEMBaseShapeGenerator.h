#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Inspector/CustomInspector.h"
#include "Utils/Utils.h"
#include <nlohmann/json.hpp>

#include <chrono>
#include <string_view>

#define BASE_SHAPE_UI_PROPERTY(x) m_RequireUpdation = x || m_RequireUpdation

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::generators
{

    typedef std::tuple<uint32_t, uint32_t, uint32_t> TextureCacheKey;

} // namespace tf3d::generators

namespace std
{
    template <>
    struct hash<tf3d::generators::TextureCacheKey> {
        std::size_t operator()(const tf3d::generators::TextureCacheKey &k) const
        {
            using std::hash;
            using std::size_t;
            using std::string;
            return ((hash<uint32_t>()(std::get<0>(k)) ^ (hash<uint32_t>()(std::get<1>(k)) << 1)) >> 1) ^ (hash<uint32_t>()(std::get<2>(k)) << 1);
        }
    };
} // namespace std

namespace tf3d::generators
{

    class DEMBaseShapeGenerator
    {
    public:
        DEMBaseShapeGenerator(ApplicationState *appState);
        ~DEMBaseShapeGenerator();

        bool ShowSettings();
        void Update(GeneratorData *buffer, GeneratorTexture *seedTexture, std::string_view profilePrefix = {});

        void Load(SerializerNode data);
        SerializerNode Save();

        inline bool RequireUpdation() const
        {
            return m_RequireUpdation;
        }
        inline bool HasTileLoaded(uint32_t x, uint32_t y, uint32_t z) const
        {
            return m_TextureCache.find(TextureCacheKey(x, y, z)) != m_TextureCache.end();
        }
        inline std::shared_ptr<Texture2D> GetTile(uint32_t x, uint32_t y, uint32_t z)
        {
            if (!HasTileLoaded(x, y, z))
                return LoadTile(x, y, z);
            return m_TextureCache.at(TextureCacheKey(x, y, z));
        }

        static bool IsTileValid(uint32_t x, uint32_t y, uint32_t z);

        std::shared_ptr<Texture2D> LoadTile(uint32_t x, uint32_t y, uint32_t z);

    private:
        void DownloadTerrainRGBTexture(TextureCacheKey key);
        void MarkViewInteraction();
        std::shared_ptr<Texture2D> FindBestAvailableTile(uint32_t x, uint32_t y, uint32_t z, TextureCacheKey &resolvedKey);
        int32_t GetEffectiveZoomResolution() const;
        void GetVisibleTileRange(int32_t zoomResolution, int32_t &minTileX, int32_t &maxTileX, int32_t &minTileY, int32_t &maxTileY) const;
        int32_t GetVisibleTileCount(int32_t zoomResolution) const;

    private:
        ApplicationState *m_AppState                                    = nullptr;
        int32_t m_ZoomResolution                                        = 0;
        int32_t m_EffectiveZoomResolution                               = 0;
        float m_ZoomOnMap                                               = 1.0f;
        float m_MapStrength                                             = 1.0f;
        int m_TilesUsingCount                                           = 0;
        int m_TilesFallbackCount                                        = 0;
        int m_VisibleTileCount                                          = 0;
        int m_TilesSkippedCount                                         = 0;
        glm::vec2 m_MapCenter                                           = glm::vec2(0.0f);
        bool m_AutoZoomResolution                                       = true;
        int m_RequestsScheduledThisUpdate                               = 0;
        std::chrono::steady_clock::time_point m_NextTileRequestTime     = std::chrono::steady_clock::time_point::min();
        std::chrono::steady_clock::time_point m_LastViewInteractionTime = std::chrono::steady_clock::time_point::min();
        bool m_ViewInteractionPending                                   = false;
        bool m_AllowTileRequestsThisUpdate                              = true;

        bool m_RequireUpdation = true;
        std::shared_ptr<GeneratorTexture> m_MapVisualzeTexture;
        std::shared_ptr<base::Texture2D> m_LoadingTexture;
        std::shared_ptr<base::Texture2D> m_NullTexture;
        std::vector<TextureCacheKey> m_TextureDownloadQueue;
        std::optional<base::ComputeShader> m_Shader;

        std::unordered_map<TextureCacheKey, std::shared_ptr<base::Texture2D>> m_TextureCache;
        std::unordered_map<TextureCacheKey, std::chrono::steady_clock::time_point> m_TileRetryAfter;
        std::string m_APIKey = "";
        char m_APIKeyInput[1024];
        std::string m_APIKeyConfigPath;
        std::string m_APIHostURL;
        std::string m_APIPathURLFormat;
        std::string m_TerrainRGBDataCacheDir;
        std::string m_TerrainRGBDataCacheFileFormat;

        static constexpr int32_t kMaxTileZoom                     = 12;
        static constexpr int32_t kMaxVisibleTiles                 = 64;
        static constexpr int32_t kMaxPendingTileRequests          = 8;
        static constexpr int32_t kMaxRequestsPerRefresh           = 4;
        static constexpr int32_t kTileRequestIntervalMilliseconds = 300;
        static constexpr int32_t kTileRequestDebounceMilliseconds = 300;
    };

} // namespace tf3d::generators
using tf3d::generators::DEMBaseShapeGenerator;
