#include "MCP/MainThreadRequestQueue.h"

#include <exception>

namespace tf3d::mcp_layer
{

    MainThreadRequestQueue::MainThreadRequestQueue()
        : ownerThread(std::this_thread::get_id())
    {
    }

    MainThreadRequestQueue::~MainThreadRequestQueue()
    {
        Shutdown();
    }

    McpResult MainThreadRequestQueue::Execute(Task task, std::chrono::milliseconds timeout)
    {
        if (!task)
            return McpResult::Failure("invalid_task", "MCP request did not contain executable work");
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stopped)
                return McpResult::Failure("shutdown", "TerraForge3D is shutting down");
        }

        if (std::this_thread::get_id() == ownerThread) {
            try {
                return task();
            } catch (const std::exception &exception) {
                return McpResult::Failure("task_exception", exception.what());
            } catch (...) {
                return McpResult::Failure("task_exception", "MCP main-thread task failed");
            }
        }

        auto pending                  = std::make_shared<PendingRequest>();
        pending->task                 = std::move(task);
        std::future<McpResult> result = pending->promise.get_future();

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stopped) {
                return McpResult::Failure("shutdown", "TerraForge3D is shutting down");
            }
            requests.push(pending);
        }

        if (result.wait_for(timeout) != std::future_status::ready) {
            pending->cancelled.store(true, std::memory_order_release);
            return McpResult::Failure("timeout", "TerraForge3D did not complete the MCP request before its deadline");
        }

        return result.get();
    }

    size_t MainThreadRequestQueue::Drain(size_t maxTasks)
    {
        if (std::this_thread::get_id() != ownerThread)
            return 0;

        size_t processed = 0;
        while (processed < maxTasks) {
            std::shared_ptr<PendingRequest> pending;
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (requests.empty())
                    break;
                pending = std::move(requests.front());
                requests.pop();
            }

            if (!pending->cancelled.load(std::memory_order_acquire)) {
                try {
                    pending->promise.set_value(pending->task());
                } catch (const std::exception &exception) {
                    pending->promise.set_value(McpResult::Failure("task_exception", exception.what()));
                } catch (...) {
                    pending->promise.set_value(McpResult::Failure("task_exception", "MCP main-thread task failed"));
                }
            } else {
                pending->promise.set_value(McpResult::Failure("cancelled", "MCP request expired before execution"));
            }

            ++processed;
        }

        return processed;
    }

    void MainThreadRequestQueue::Shutdown()
    {
        std::queue<std::shared_ptr<PendingRequest>> pendingRequests;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stopped && requests.empty())
                return;
            stopped = true;
            pendingRequests.swap(requests);
        }

        while (!pendingRequests.empty()) {
            pendingRequests.front()->cancelled.store(true, std::memory_order_release);
            pendingRequests.front()->promise.set_value(
                McpResult::Failure("shutdown", "TerraForge3D is shutting down"));
            pendingRequests.pop();
        }
    }

    bool MainThreadRequestQueue::IsShutdown() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return stopped;
    }

} // namespace tf3d::mcp_layer
