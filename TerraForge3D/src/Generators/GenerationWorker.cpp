#include "Generators/GenerationWorker.h"

#include "Profiler.h"
#include "Utils/Utils.h"

#include <GLFW/glfw3.h>

#include <chrono>
#include <exception>

namespace tf3d::generators
{

    GenerationWorker::GenerationWorker(std::string name, WorkCallback callback, std::string profilePrefix)
        : m_Name(name.empty() ? "Generation Worker" : std::move(name)),
          m_ProfilePrefix(profilePrefix.empty() ? "generation" : std::move(profilePrefix)),
          m_Callback(std::move(callback))
    {
        GLFWwindow *renderWindow = glfwGetCurrentContext();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        m_Window = glfwCreateWindow(1, 1, "TerraForge3D Generation", nullptr, renderWindow);
        glfwMakeContextCurrent(renderWindow);
        if (m_Window != nullptr) {
            m_Thread = std::thread(&GenerationWorker::Run, this);
        } else {
            TF3D_LOG_ERROR("Failed to create shared generation OpenGL context; generation will run on the render thread");
        }
    }

    GenerationWorker::~GenerationWorker()
    {
        {
            std::lock_guard lock(m_Mutex);
            m_StopRequested = true;
        }
        m_Condition.notify_one();

        if (m_Thread.joinable())
            m_Thread.join();
        if (m_Window != nullptr) {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    GenerationWorker::RequestResult GenerationWorker::Request()
    {
        if (!HasContext()) {
            return {RequestStatus::Unavailable, 0};
        }

        uint64_t requestId = 0;
        {
            std::lock_guard lock(m_Mutex);
            if (m_StopRequested.load(std::memory_order_acquire) ||
                m_RequestPending.load(std::memory_order_acquire) ||
                m_Running.load(std::memory_order_acquire) ||
                m_GpuCompletionPending ||
                m_Completed.load(std::memory_order_acquire)) {
                return {RequestStatus::Busy, 0};
            }
            requestId        = NextUniqueId();
            m_LastRequestId  = requestId;
            m_RequestPending = true;
        }
        m_Condition.notify_one();
        return {RequestStatus::Queued, requestId};
    }

    bool GenerationWorker::CanAcceptRequest()
    {
        if (!HasContext()) {
            return true;
        }

        std::lock_guard lock(m_Mutex);
        return !m_StopRequested.load(std::memory_order_acquire) &&
               !m_RequestPending.load(std::memory_order_acquire) &&
               !m_Running.load(std::memory_order_acquire) &&
               !m_GpuCompletionPending &&
               !m_Completed.load(std::memory_order_acquire);
    }

    bool GenerationWorker::PollCompletion()
    {
        if (!HasContext() || glfwGetCurrentContext() == nullptr)
            return false;

        {
            std::lock_guard lock(m_Mutex);
            if (!m_GpuCompletionPending || m_CompletionFence == nullptr)
                return false;

            const GLenum waitResult = glClientWaitSync(
                m_CompletionFence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
            if (waitResult == GL_TIMEOUT_EXPIRED)
                return false;

            if (waitResult == GL_WAIT_FAILED) {
                TF3D_LOG_ERROR("OpenGL completion fence failed for generation worker '{}'", m_Name);
                glFinish();
            }

            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glDeleteSync(m_CompletionFence);
            m_CompletionFence      = nullptr;
            m_GpuCompletionPending = false;
            m_CompletedRequestId   = m_CompletionRequestId;
            m_Completed            = true;
            m_GpuPollRequested     = true;
            m_Running              = false;
            m_ActiveRequestId      = 0;
        }
        m_Condition.notify_all();

        return true;
    }

    void GenerationWorker::WaitForIdle()
    {
        TF3D_PROFILE_SCOPE_LAZY_DOMAIN(m_ProfilePrefix + "/wait-for-idle", PerformanceMonitor::Domain::Wait);
        while (true) {
            PollCompletion();
            std::unique_lock lock(m_Mutex);
            const bool idle = !m_Running.load(std::memory_order_acquire) &&
                              !m_RequestPending.load(std::memory_order_acquire) &&
                              !m_GpuCompletionPending && !m_GpuPollRequested;
            if (idle)
                break;
            m_Condition.wait_for(lock, std::chrono::milliseconds(1));
        }
    }

    bool GenerationWorker::ConsumeCompleted()
    {
        const bool consumed = m_Completed.exchange(false, std::memory_order_acq_rel);
        if (consumed)
            m_Condition.notify_all();
        return consumed;
    }

    void GenerationWorker::Run()
    {
        glfwMakeContextCurrent(m_Window);
        TF3D_PROFILE_THREAD_NAME(m_Name);
        while (true) {
            uint64_t requestId     = 0;
            bool pollGpuQueries    = false;
            GLsync completionFence = nullptr;
            {
                std::unique_lock lock(m_Mutex);
                m_Condition.wait(lock, [this] {
                    return m_StopRequested.load(std::memory_order_acquire) ||
                           m_GpuPollRequested ||
                           (m_RequestPending.load(std::memory_order_acquire) &&
                            !m_GpuCompletionPending &&
                            !m_Completed.load(std::memory_order_acquire));
                });
                if (m_StopRequested.load(std::memory_order_acquire)) {
                    if (m_CompletionFence != nullptr) {
                        const GLenum waitResult = glClientWaitSync(
                            m_CompletionFence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
                        if (waitResult == GL_WAIT_FAILED)
                            TF3D_LOG_ERROR("OpenGL completion fence failed while stopping generation worker '{}'", m_Name);
                        glDeleteSync(m_CompletionFence);
                        m_CompletionFence = nullptr;
                    }
                    m_GpuCompletionPending = false;
                    m_GpuPollRequested     = false;
                    m_Running              = false;
                    m_ActiveRequestId      = 0;
                    break;
                }

                if (m_GpuPollRequested) {
                    m_GpuPollRequested = false;
                    m_Running          = true;
                    pollGpuQueries     = true;
                } else {
                    requestId        = m_LastRequestId.load(std::memory_order_acquire);
                    m_RequestPending = false;
                    m_Running        = true;
                }
            }

            if (pollGpuQueries) {
                TF3D_PROFILE_POLL_GPU();
                {
                    std::lock_guard lock(m_Mutex);
                    m_Running = false;
                }
                m_Condition.notify_all();
                continue;
            }

            m_ActiveRequestId        = requestId;
            {
                TF3D_PROFILE_SCOPE_LAZY_DOMAIN(m_ProfilePrefix + "/execute", PerformanceMonitor::Domain::Worker);
                try {
                    if (m_Callback)
                        m_Callback(requestId);
                } catch (const std::exception &exception) {
                    TF3D_LOG_ERROR("{} callback failed: {}", m_Name, exception.what());
                } catch (...) {
                    TF3D_LOG_ERROR("{} callback failed with an unknown exception", m_Name);
                }
            }

            {
                TF3D_PROFILE_SCOPE_LAZY_DOMAIN(m_ProfilePrefix + "/synchronize", PerformanceMonitor::Domain::Wait);
                glMemoryBarrier(GL_ALL_BARRIER_BITS);
                completionFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                if (completionFence != nullptr) {
                    glFlush();
                } else {
                    TF3D_LOG_ERROR("Failed to create OpenGL completion fence for generation worker '{}'", m_Name);
                    glFinish();
                }
            }

            if (completionFence != nullptr) {
                {
                    std::lock_guard lock(m_Mutex);
                    m_CompletionFence      = completionFence;
                    m_CompletionRequestId  = requestId;
                    m_GpuCompletionPending = true;
                }
                continue;
            }

            TF3D_PROFILE_POLL_GPU();

            {
                std::lock_guard lock(m_Mutex);
                m_Running            = false;
                m_Completed          = true;
                m_CompletedRequestId = requestId;
                m_ActiveRequestId    = 0;
            }
            m_Condition.notify_all();
        }
        glfwMakeContextCurrent(nullptr);
    }

} // namespace tf3d::generators
