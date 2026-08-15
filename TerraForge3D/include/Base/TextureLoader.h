#pragma once

#include "Base/Base.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

TF3D_FWD_DEC_CLASS(Texture2D, tf3d::base)

struct GLFWwindow;

namespace tf3d::base
{

    enum class TextureLoadPriority : int32_t {
        Low    = 0,
        Normal = 100,
        High   = 200
    };

    struct TextureLoadOptions {
        bool preserveData = false;
        bool readAlpha    = false;
        bool loadAs16Bit  = false;

        constexpr bool operator==(const TextureLoadOptions &) const = default;
    };

    struct TextureLoadResult {
        uint64_t requestId = 0;
        std::string path;
        std::shared_ptr<Texture2D> texture;

        inline bool Succeeded() const
        {
            return texture != nullptr;
        }
    };

    class TextureLoader
    {
    public:
        using DecodeFunction    = std::function<std::shared_ptr<Texture2D>(const std::string &)>;
        using CompletionHandler = std::function<void(TextureLoadResult)>;

        TextureLoader();
        ~TextureLoader();

        TextureLoader(const TextureLoader &)            = delete;
        TextureLoader &operator=(const TextureLoader &) = delete;

        inline uint64_t Request(std::string path,
                                TextureLoadPriority priority,
                                CompletionHandler handler,
                                TextureLoadOptions options = {})
        {
            return RequestImpl(std::move(path),
                               priority,
                               {},
                               std::move(handler),
                               options,
                               false);
        }

        inline uint64_t Request(std::string path,
                                TextureLoadPriority priority,
                                DecodeFunction decoder,
                                CompletionHandler handler)
        {
            return RequestImpl(std::move(path),
                               priority,
                               std::move(decoder),
                               std::move(handler),
                               {},
                               true);
        }

        void ProcessCompletions();

        bool HasContext() const;
        bool IsPending(const std::string &path) const;
        std::size_t PendingCount() const;

    private:
        struct LoadKey {
            std::string path;
            TextureLoadOptions options;
            bool customDecoder = false;

            bool operator==(const LoadKey &) const = default;
        };

        struct LoadKeyHash {
            std::size_t operator()(const LoadKey &key) const noexcept;
        };

        struct PendingLoad {
            LoadKey key;
            TextureLoadPriority priority = TextureLoadPriority::Normal;
            uint64_t requestId           = 0;
            uint64_t sequence            = 0;
            uint64_t queueVersion        = 0;
            DecodeFunction decoder;
            std::vector<CompletionHandler> handlers;
        };

        struct QueueEntry {
            uint64_t requestId           = 0;
            TextureLoadPriority priority = TextureLoadPriority::Normal;
            uint64_t sequence            = 0;
            uint64_t queueVersion        = 0;
        };

        struct QueueEntryCompare {
            bool operator()(const QueueEntry &left, const QueueEntry &right) const;
        };

        struct Completion {
            CompletionHandler handler;
            TextureLoadResult result;
        };

        static LoadKey MakeKey(const std::string &path,
                               const TextureLoadOptions &options,
                               bool customDecoder);
        static std::shared_ptr<Texture2D> LoadTexture(const LoadKey &key,
                                                      const DecodeFunction &decoder);

        uint64_t RequestImpl(std::string path,
                             TextureLoadPriority priority,
                             DecodeFunction decoder,
                             CompletionHandler handler,
                             TextureLoadOptions options,
                             bool customDecoder);
        void Run();

        GLFWwindow *m_Window = nullptr;
        std::thread m_WorkerThread;
        mutable std::mutex m_Mutex;
        std::condition_variable m_Condition;
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, QueueEntryCompare> m_Queue;
        std::unordered_map<uint64_t, PendingLoad> m_Pending;
        std::unordered_map<LoadKey, uint64_t, LoadKeyHash> m_PendingByKey;
        std::deque<Completion> m_Completions;
        bool m_StopRequested                = false;
        std::atomic_bool m_ContextAvailable = false;
        std::atomic<uint64_t> m_NextRequestId{1};
        uint64_t m_NextSequence = 1;
    };

} // namespace tf3d::base
