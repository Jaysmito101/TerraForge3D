#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

struct GLFWwindow;

class GenerationWorker
{
public:
    using WorkCallback = std::function<void(bool force)>;

    GenerationWorker(std::string name, WorkCallback callback);
    ~GenerationWorker();

    GenerationWorker(const GenerationWorker &)            = delete;
    GenerationWorker &operator=(const GenerationWorker &) = delete;

    bool Request(bool force);
    void WaitForIdle();
    bool ConsumeCompleted();

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
    std::atomic_bool m_RequestPending = false;
    bool m_ForceRequested             = false;
    std::atomic_bool m_Running        = false;
    std::atomic_bool m_Completed      = false;
    std::atomic_bool m_StopRequested  = false;
    std::string m_Name;
    WorkCallback m_Callback;
};
