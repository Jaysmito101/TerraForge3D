#pragma once

#include "MCP/McpResult.h"

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

class MainThreadRequestQueue
{
public:
    using Task = std::function<McpResult()>;

    MainThreadRequestQueue();
    ~MainThreadRequestQueue();

    McpResult Execute(
        Task task,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    std::size_t Drain(std::size_t maxTasks = 64);
    void Shutdown();
    bool IsShutdown() const;

private:
    struct PendingRequest {
        Task task;
        std::promise<McpResult> promise;
        std::atomic<bool> cancelled = false;
    };

    std::thread::id ownerThread;
    mutable std::mutex mutex;
    std::queue<std::shared_ptr<PendingRequest>> requests;
    bool stopped = false;
};
