#pragma once

#include "API/ApiResult.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace tf3d::api
{

    class MainThreadRequestQueue
    {
    public:
        using Task = std::function<ApiResult()>;

        MainThreadRequestQueue();
        ~MainThreadRequestQueue();

        ApiResult Execute(
            Task task,
            std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

        std::size_t Drain(std::size_t maxTasks = 64);
        void Shutdown();
        bool IsShutdown() const;

    private:
        struct PendingRequest {
            Task task;
            std::promise<ApiResult> promise;
            std::atomic<bool> cancelled = false;
        };

        std::thread::id ownerThread;
        mutable std::mutex mutex;
        std::queue<std::shared_ptr<PendingRequest>> requests;
        bool stopped = false;
    };

} // namespace tf3d::api
