#include "API/MainThreadRequestQueue.h"

#include <exception>

namespace tf3d::api
{

    MainThreadRequestQueue::MainThreadRequestQueue()
        : ownerThread(std::this_thread::get_id())
    {
    }

    MainThreadRequestQueue::~MainThreadRequestQueue()
    {
        Shutdown();
    }

    ApiResult MainThreadRequestQueue::Execute(Task task, std::chrono::milliseconds timeout)
    {
        if (!task) {
            return ApiResult::Failure(ApiErrorType::InvalidTask,
                                      "API request did not contain executable work");
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stopped) {
                return ApiResult::Failure(ApiErrorType::Shutdown,
                                          "TerraForge3D is shutting down");
            }
        }

        if (std::this_thread::get_id() == ownerThread) {
            try {
                return task();
            } catch (const std::exception &exception) {
                return ApiResult::Failure(ApiErrorType::TaskException, exception.what());
            } catch (...) {
                return ApiResult::Failure(ApiErrorType::TaskException,
                                          "API main-thread task failed");
            }
        }

        auto pending                  = std::make_shared<PendingRequest>();
        pending->task                 = std::move(task);
        std::future<ApiResult> result = pending->promise.get_future();

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stopped) {
                return ApiResult::Failure(ApiErrorType::Shutdown,
                                          "TerraForge3D is shutting down");
            }
            requests.push(pending);
        }

        if (result.wait_for(timeout) != std::future_status::ready) {
            pending->cancelled.store(true, std::memory_order_release);
            return ApiResult::Failure(
                ApiErrorType::Timeout,
                "TerraForge3D did not complete the API request before its deadline");
        }

        return result.get();
    }

    std::size_t MainThreadRequestQueue::Drain(std::size_t maxTasks)
    {
        if (std::this_thread::get_id() != ownerThread) {
            return 0;
        }

        std::size_t processed = 0;
        while (processed < maxTasks) {
            std::shared_ptr<PendingRequest> pending;
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (requests.empty()) {
                    break;
                }
                pending = std::move(requests.front());
                requests.pop();
            }

            if (!pending->cancelled.load(std::memory_order_acquire)) {
                try {
                    pending->promise.set_value(pending->task());
                } catch (const std::exception &exception) {
                    pending->promise.set_value(
                        ApiResult::Failure(ApiErrorType::TaskException, exception.what()));
                } catch (...) {
                    pending->promise.set_value(
                        ApiResult::Failure(ApiErrorType::TaskException,
                                           "API main-thread task failed"));
                }
            } else {
                pending->promise.set_value(
                    ApiResult::Failure(ApiErrorType::Cancelled,
                                       "API request expired before execution"));
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
            if (stopped && requests.empty()) {
                return;
            }
            stopped = true;
            pendingRequests.swap(requests);
        }

        while (!pendingRequests.empty()) {
            pendingRequests.front()->cancelled.store(true, std::memory_order_release);
            pendingRequests.front()->promise.set_value(
                ApiResult::Failure(ApiErrorType::Shutdown,
                                   "TerraForge3D is shutting down"));
            pendingRequests.pop();
        }
    }

    bool MainThreadRequestQueue::IsShutdown() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return stopped;
    }

} // namespace tf3d::api
