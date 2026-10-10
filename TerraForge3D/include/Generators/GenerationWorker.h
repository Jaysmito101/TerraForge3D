#pragma once

#include "Base/SyncFence.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

struct GLFWwindow;

namespace tf3d::generators
{

    class GenerationWorker
    {
    public:
        enum class RequestStatus {
            Queued,
            Busy,
            Unavailable
        };

        struct RequestResult {
            RequestStatus status = RequestStatus::Unavailable;
            uint64_t requestId   = 0;
        };

        using WorkCallback = std::function<void(uint64_t requestId)>;

        GenerationWorker(std::string name, WorkCallback callback, std::string profilePrefix = "generation");
        ~GenerationWorker();

        GenerationWorker(const GenerationWorker &)            = delete;
        GenerationWorker &operator=(const GenerationWorker &) = delete;

        RequestResult Request();
        bool CanAcceptRequest();
        bool Poll();
        void WaitForIdle();
        std::optional<uint64_t> TryConsumeCompleted();

        inline uint64_t GetLastRequestId() const
        {
            return m_Request.lastRequestId.load(std::memory_order_acquire);
        }

        inline uint64_t GetActiveRequestId() const
        {
            return m_Request.activeRequestId.load(std::memory_order_acquire);
        }

        inline bool HasContext() const
        {
            return m_Context.window != nullptr;
        }
        inline bool IsRunning() const
        {
            const WorkerPhase phase = m_Request.phase.load(std::memory_order_acquire);
            return phase == WorkerPhase::ExecutingRequest ||
                   phase == WorkerPhase::AwaitingGpuCompletion ||
                   phase == WorkerPhase::ProfilerDrainQueued ||
                   phase == WorkerPhase::DrainingGpuProfiler;
        }
        inline bool IsRequestPending() const
        {
            return m_Request.phase.load(std::memory_order_acquire) == WorkerPhase::RequestQueued;
        }
    private:
        enum class WorkerPhase {
            Idle,
            RequestQueued,
            ExecutingRequest,
            AwaitingGpuCompletion,
            ProfilerDrainQueued,
            DrainingGpuProfiler,
            CompletionReady,
            StopRequested,
            Stopped
        };

        struct WorkerConfig {
            std::string name;
            std::string profilePrefix;
            WorkCallback callback;
        };

        struct ThreadContext {
            GLFWwindow *window = nullptr;
            std::thread thread;
        };

        struct RequestState {
            std::atomic<WorkerPhase> phase{WorkerPhase::Idle};
            std::optional<base::SyncFence> completionFence;

            std::atomic<uint64_t> lastRequestId      = 0;
            std::atomic<uint64_t> activeRequestId    = 0;
            uint64_t completedRequestId = 0;
        };

        enum class WorkItemType {
            Stop,
            DrainGpuProfiler,
            GenerateRequest
        };

        struct WorkItem {
            WorkItemType type;
            uint64_t requestId = 0;
        };

        void Run();
        WorkItem WaitForWorkItem();
        void HandleStop(std::unique_lock<std::mutex> &);
        void ExecuteRequest(uint64_t requestId);

        WorkerConfig m_Config;
        ThreadContext m_Context;

        // m_Mutex protects request transitions, the completed request ID, and the completion fence.
        std::mutex m_Mutex;
        std::condition_variable m_Condition;

        RequestState m_Request;
    };

} // namespace tf3d::generators
