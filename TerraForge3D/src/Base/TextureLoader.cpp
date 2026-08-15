#include "Base/TextureLoader.h"

#include "Base/Logging/Logger.h"
#include "Base/Texture2D.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <exception>
#include <utility>

namespace tf3d::base
{

    TextureLoader::TextureLoader()
    {
        GLFWwindow *renderWindow = glfwGetCurrentContext();
        if (renderWindow == nullptr) {
            TF3D_LOG_ERROR("Cannot start texture loader without a current OpenGL context");
            return;
        }

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        m_Window = glfwCreateWindow(1, 1, "TerraForge3D Texture Loader", nullptr, renderWindow);
        glfwMakeContextCurrent(renderWindow);
        if (m_Window == nullptr) {
            TF3D_LOG_ERROR("Failed to create shared texture-loader OpenGL context");
            return;
        }

        m_ContextAvailable = true;
        m_WorkerThread     = std::thread(&TextureLoader::Run, this);
    }

    TextureLoader::~TextureLoader()
    {
        {
            std::lock_guard lock(m_Mutex);
            m_StopRequested = true;
            while (!m_Queue.empty()) {
                m_Queue.pop();
            }
        }
        m_Condition.notify_all();

        if (m_WorkerThread.joinable()) {
            m_WorkerThread.join();
        }

        {
            std::lock_guard lock(m_Mutex);
            m_Pending.clear();
            m_PendingByKey.clear();
            m_Completions.clear();
        }

        m_ContextAvailable = false;
        if (m_Window != nullptr) {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    uint64_t TextureLoader::RequestImpl(std::string path,
                                        TextureLoadPriority priority,
                                        DecodeFunction decoder,
                                        CompletionHandler handler,
                                        TextureLoadOptions options,
                                        bool customDecoder)
    {
        const uint64_t requestId = m_NextRequestId.fetch_add(1, std::memory_order_relaxed);
        if (path.empty()) {
            if (handler) {
                std::lock_guard lock(m_Mutex);
                m_Completions.push_back({std::move(handler), {requestId, std::move(path), nullptr}});
            }
            m_Condition.notify_one();
            return requestId;
        }

        const LoadKey key = MakeKey(path, options, customDecoder);
        {
            std::lock_guard lock(m_Mutex);
            const auto keyPending = m_PendingByKey.find(key);
            if (keyPending != m_PendingByKey.end()) {
                const auto pending = m_Pending.find(keyPending->second);
                if (pending != m_Pending.end()) {
                    auto &request = pending->second;
                    if (handler) {
                        request.handlers.emplace_back(std::move(handler));
                    }

                    if (static_cast<int32_t>(priority) > static_cast<int32_t>(request.priority)) {
                        request.priority = priority;
                        ++request.queueVersion;
                        m_Queue.push({request.requestId,
                                      request.priority,
                                      request.sequence,
                                      request.queueVersion});
                    }
                    return request.requestId;
                }
                m_PendingByKey.erase(keyPending);
            }

            if (!m_ContextAvailable.load(std::memory_order_acquire)) {
                if (handler) {
                    m_Completions.push_back({std::move(handler), {requestId, std::move(path), nullptr}});
                }
                return requestId;
            }

            PendingLoad request{};
            request.key          = key;
            request.priority     = priority;
            request.requestId    = requestId;
            request.sequence     = m_NextSequence++;
            request.decoder      = std::move(decoder);
            request.queueVersion = 1;
            const uint64_t sequence = request.sequence;
            if (handler) {
                request.handlers.emplace_back(std::move(handler));
            }

            m_Pending.emplace(request.requestId, std::move(request));
            m_PendingByKey.emplace(key, requestId);
            m_Queue.push({requestId, priority, sequence, 1});
        }

        m_Condition.notify_one();
        return requestId;
    }

    void TextureLoader::ProcessCompletions()
    {
        std::deque<Completion> completions;
        {
            std::lock_guard lock(m_Mutex);
            completions.swap(m_Completions);
        }

        for (auto &completion : completions) {
            if (!completion.handler) {
                continue;
            }

            try {
                completion.handler(std::move(completion.result));
            } catch (const std::exception &exception) {
                TF3D_LOG_ERROR("Texture load completion handler failed: {}", exception.what());
            } catch (...) {
                TF3D_LOG_ERROR("Texture load completion handler failed with an unknown exception");
            }
        }
    }

    bool TextureLoader::HasContext() const
    {
        return m_ContextAvailable.load(std::memory_order_acquire);
    }

    bool TextureLoader::IsPending(const std::string &path) const
    {
        std::lock_guard lock(m_Mutex);
        for (const auto &[key, requestId] : m_PendingByKey) {
            if (key.path == path && m_Pending.contains(requestId)) {
                return true;
            }
        }
        return false;
    }

    std::size_t TextureLoader::PendingCount() const
    {
        std::lock_guard lock(m_Mutex);
        return m_Pending.size();
    }

    TextureLoader::LoadKey TextureLoader::MakeKey(const std::string &path,
                                                  const TextureLoadOptions &options,
                                                  bool customDecoder)
    {
        return {path, options, customDecoder};
    }

    std::shared_ptr<Texture2D> TextureLoader::LoadTexture(const LoadKey &key,
                                                          const DecodeFunction &decoder)
    {
        try {
            if (decoder) {
                return decoder(key.path);
            }

            auto texture = std::make_shared<Texture2D>(key.path,
                                                       key.options.preserveData,
                                                       key.options.readAlpha,
                                                       key.options.loadAs16Bit);
            if (!texture->IsLoaded()) {
                return nullptr;
            }
            return texture;
        } catch (const std::exception &exception) {
            TF3D_LOG_ERROR("Texture load failed for '{}': {}", key.path, exception.what());
        } catch (...) {
            TF3D_LOG_ERROR("Texture load failed for '{}' with an unknown exception", key.path);
        }
        return nullptr;
    }

    std::size_t TextureLoader::LoadKeyHash::operator()(const LoadKey &key) const noexcept
    {
        std::size_t hash = std::hash<std::string>{}(key.path);
        hash ^= std::hash<bool>{}(key.options.preserveData) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
        hash ^= std::hash<bool>{}(key.options.readAlpha) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
        hash ^= std::hash<bool>{}(key.options.loadAs16Bit) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
        hash ^= std::hash<bool>{}(key.customDecoder) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
        return hash;
    }

    bool TextureLoader::QueueEntryCompare::operator()(const QueueEntry &left,
                                                      const QueueEntry &right) const
    {
        if (left.priority != right.priority) {
            return static_cast<int32_t>(left.priority) < static_cast<int32_t>(right.priority);
        }
        return left.sequence > right.sequence;
    }

    void TextureLoader::Run()
    {
        glfwMakeContextCurrent(m_Window);
        while (true) {
            uint64_t requestId = 0;
            LoadKey requestKey{};
            DecodeFunction decoder{};
            {
                std::unique_lock lock(m_Mutex);
                m_Condition.wait(lock, [this] {
                    return m_StopRequested || !m_Queue.empty();
                });

                if (m_StopRequested) {
                    break;
                }

                const QueueEntry entry = m_Queue.top();
                m_Queue.pop();
                if (entry.requestId == 0) {
                    continue;
                }

                const auto pending = m_Pending.find(entry.requestId);
                if (pending == m_Pending.end() ||
                    pending->second.queueVersion != entry.queueVersion) {
                    continue;
                }
                requestId = entry.requestId;
                requestKey = pending->second.key;
                decoder   = pending->second.decoder;
            }

            const auto texture = LoadTexture(requestKey, decoder);
            if (texture != nullptr) {
                glFlush();
            }

            {
                std::lock_guard lock(m_Mutex);
                if (m_StopRequested) {
                    break;
                }

                const auto pending = m_Pending.find(requestId);
                if (pending == m_Pending.end()) {
                    continue;
                }

                auto &pendingRequest = pending->second;
                auto handlers        = std::move(pendingRequest.handlers);
                const auto path       = pendingRequest.key.path;
                m_PendingByKey.erase(pendingRequest.key);
                m_Pending.erase(pending);
                for (auto &handler : handlers) {
                    m_Completions.push_back({std::move(handler),
                                             {requestId, path, texture}});
                }
            }
        }
        glfwMakeContextCurrent(nullptr);
    }

} // namespace tf3d::base
