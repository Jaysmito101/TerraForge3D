#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <glad/gl.h>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

struct GLFWwindow;

namespace tf3d::generators
{

    class GenerationWorker
    {
    public:
        using WorkCallback = std::function<void(bool force, uint64_t requestId)>;

        GenerationWorker(std::string name, WorkCallback callback, std::string profilePrefix = "generation");
        ~GenerationWorker();

        GenerationWorker(const GenerationWorker &)            = delete;
        GenerationWorker &operator=(const GenerationWorker &) = delete;

        bool Request(bool force, uint64_t *requestIdOut = nullptr);
        bool PollCompletion();
        void WaitForIdle();
        bool ConsumeCompleted();

        inline uint64_t GetLastRequestId() const
        {
            return m_LastRequestId.load(std::memory_order_acquire);
        }

        inline uint64_t GetActiveRequestId() const
        {
            return m_ActiveRequestId.load(std::memory_order_acquire);
        }

        inline uint64_t GetCompletedRequestId() const
        {
            return m_CompletedRequestId.load(std::memory_order_acquire);
        }

        inline bool HasContext() const
        {
            return m_Window != nullptr;
        }
        inline bool IsRunning() const
        {
            return m_Running.load(std::memory_order_acquire);
        }
        inline bool IsRequestPending() const
        {
            return m_RequestPending.load(std::memory_order_acquire);
        }
        inline bool IsCompleted() const
        {
            return m_Completed.load(std::memory_order_acquire);
        }

    private:
        void Run();

        GLFWwindow *m_Window = nullptr;
        std::thread m_Thread;
        std::mutex m_Mutex;
        std::condition_variable m_Condition;
        std::atomic_bool m_RequestPending          = false;
        bool m_ForceRequested                      = false;
        std::atomic_bool m_Running                 = false;
        std::atomic_bool m_Completed               = false;
        std::atomic_bool m_StopRequested           = false;
        GLsync m_CompletionFence                   = nullptr;
        uint64_t m_CompletionRequestId             = 0;
        bool m_GpuCompletionPending                = false;
        bool m_GpuPollRequested                    = false;
        std::atomic<uint64_t> m_LastRequestId      = 0;
        std::atomic<uint64_t> m_ActiveRequestId    = 0;
        std::atomic<uint64_t> m_CompletedRequestId = 0;
        std::string m_Name;
        std::string m_ProfilePrefix;
        WorkCallback m_Callback;
    };

} // namespace tf3d::generators
using tf3d::generators::GenerationWorker;
