#include "Generators/GenerationWorker.h"

#include "Profiler.h"
#include "Utils/Utils.h"

#include <GLFW/glfw3.h>

#include <chrono>
#include <exception>
#include <utility>

namespace tf3d::generators
{

    GenerationWorker::GenerationWorker(std::string name, WorkCallback callback, std::string profilePrefix)
        : m_Config{name.empty() ? "Generation Worker" : std::move(name),
                   profilePrefix.empty() ? "generation" : std::move(profilePrefix),
                   std::move(callback)}
    {
        GLFWwindow *renderWindow = glfwGetCurrentContext();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        m_Context.window = glfwCreateWindow(1, 1, "TerraForge3D Generation", nullptr, renderWindow);
        glfwMakeContextCurrent(renderWindow);

        if (m_Context.window != nullptr) {
            m_Context.thread = std::thread(&GenerationWorker::Run, this);
        } else {
            TF3D_LOG_ERROR("Failed to create shared generation OpenGL context; generation will run on the render thread");
        }
    }

    GenerationWorker::~GenerationWorker()
    {
        {
            std::lock_guard lock(m_Mutex);
            m_Request.phase = WorkerPhase::StopRequested;
        }
        m_Condition.notify_one();

        if (m_Context.thread.joinable()) {
            m_Context.thread.join();
        }

        if (m_Context.window != nullptr) {
            glfwDestroyWindow(m_Context.window);
            m_Context.window = nullptr;
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
            if (m_Request.phase.load(std::memory_order_acquire) != WorkerPhase::Idle) {
                return {RequestStatus::Busy, 0};
            }

            requestId               = NextUniqueId();
            m_Request.lastRequestId = requestId;
            m_Request.phase         = WorkerPhase::RequestQueued;
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
        return m_Request.phase.load(std::memory_order_acquire) == WorkerPhase::Idle;
    }

    bool GenerationWorker::PollCompletion()
    {
        if (!HasContext() || glfwGetCurrentContext() == nullptr) {
            return false;
        }

        {
            std::lock_guard lock(m_Mutex);
            if (m_Request.phase.load(std::memory_order_acquire) != WorkerPhase::AwaitingGpuCompletion ||
                !m_Request.completionFence) {
                return false;
            }

            const GLenum waitResult = m_Request.completionFence->ClientWait(GL_SYNC_FLUSH_COMMANDS_BIT, 0);
            if (waitResult == GL_TIMEOUT_EXPIRED) {
                return false;
            }

            if (waitResult == GL_WAIT_FAILED) {
                TF3D_LOG_ERROR("OpenGL completion fence failed for generation worker '{}'", m_Config.name);
                glFinish();
            }

            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            m_Request.completedRequestId = m_Request.activeRequestId.load(std::memory_order_acquire);
            m_Request.phase              = WorkerPhase::ProfilerDrainQueued;
            m_Request.activeRequestId    = 0;
        }
        m_Condition.notify_all();

        return true;
    }

    void GenerationWorker::WaitForIdle()
    {
        TF3D_PROFILE_SCOPE_LAZY_DOMAIN(m_Config.profilePrefix + "/wait-for-idle", PerformanceMonitor::Domain::Wait);
        while (true) {
            PollCompletion();
            std::unique_lock lock(m_Mutex);
            const WorkerPhase phase = m_Request.phase.load(std::memory_order_acquire);
            const bool idle = phase == WorkerPhase::Idle || phase == WorkerPhase::CompletionReady ||
                              phase == WorkerPhase::Stopped;
            if (idle) {
                break;
            }
            m_Condition.wait_for(lock, std::chrono::milliseconds(1));
        }
    }

    bool GenerationWorker::ConsumeCompleted()
    {
        bool consumed = false;
        {
            std::lock_guard lock(m_Mutex);
            if (m_Request.phase.load(std::memory_order_acquire) == WorkerPhase::CompletionReady) {
                if (m_Request.completionFence.has_value() && glfwGetCurrentContext() == nullptr) {
                    return false;
                }
                m_Request.completionFence.reset();
                m_Request.phase = WorkerPhase::Idle;
                consumed        = true;
            }
        }
        if (consumed) {
            m_Condition.notify_all();
        }
        return consumed;
    }

    void GenerationWorker::Run()
    {
        glfwMakeContextCurrent(m_Context.window);
        TF3D_PROFILE_THREAD_NAME(m_Config.name);

        while (true) {
            const WorkItem item = WaitForWorkItem();
            switch (item.type) {
                case WorkItemType::Stop:
                    glfwMakeContextCurrent(nullptr);
                    return;
                case WorkItemType::DrainGpuProfiler: {
                    TF3D_PROFILE_DRAIN_GPU();
                    {
                        std::lock_guard lock(m_Mutex);
                        if (m_Request.phase.load(std::memory_order_acquire) != WorkerPhase::StopRequested) {
                            m_Request.phase = WorkerPhase::CompletionReady;
                        }
                    }
                    m_Condition.notify_all();
                    break;
                }
                case WorkItemType::GenerateRequest:
                    ExecuteRequest(item.requestId);
                    break;
            }
        }
    }

    GenerationWorker::WorkItem GenerationWorker::WaitForWorkItem()
    {
        std::unique_lock lock(m_Mutex);
        m_Condition.wait(lock, [this] {
            const WorkerPhase phase = m_Request.phase.load(std::memory_order_acquire);
            return phase == WorkerPhase::StopRequested || phase == WorkerPhase::ProfilerDrainQueued ||
                   phase == WorkerPhase::RequestQueued;
        });

        if (m_Request.phase.load(std::memory_order_acquire) == WorkerPhase::StopRequested) {
            HandleStop(lock);
            return {WorkItemType::Stop};
        }

        if (m_Request.phase.load(std::memory_order_acquire) == WorkerPhase::ProfilerDrainQueued) {
            m_Request.phase = WorkerPhase::DrainingGpuProfiler;
            return {WorkItemType::DrainGpuProfiler};
        }

        const uint64_t requestId = m_Request.lastRequestId.load(std::memory_order_acquire);
        m_Request.activeRequestId = requestId;
        m_Request.phase           = WorkerPhase::ExecutingRequest;
        return {WorkItemType::GenerateRequest, requestId};
    }

    void GenerationWorker::HandleStop(std::unique_lock<std::mutex> &)
    {
        if (m_Request.completionFence.has_value()) {
            const GLenum waitResult =
                m_Request.completionFence->ClientWait(GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
            if (waitResult == GL_WAIT_FAILED) {
                TF3D_LOG_ERROR("OpenGL completion fence failed while stopping generation worker '{}'", m_Config.name);
            }
            m_Request.completionFence.reset();
        }
        m_Request.phase           = WorkerPhase::Stopped;
        m_Request.activeRequestId = 0;
    }

    void GenerationWorker::ExecuteRequest(uint64_t requestId)
    {
        {
            TF3D_PROFILE_SCOPE_LAZY_DOMAIN(m_Config.profilePrefix + "/execute", PerformanceMonitor::Domain::Worker);
            try {
                if (m_Config.callback) {
                    m_Config.callback(requestId);
                }
            } catch (const std::exception &exception) {
                TF3D_LOG_ERROR("{} callback failed: {}", m_Config.name, exception.what());
            } catch (...) {
                TF3D_LOG_ERROR("{} callback failed with an unknown exception", m_Config.name);
            }
        }

        base::SyncFence completionFence(GL_ALL_BARRIER_BITS);
        if (completionFence) {
            std::lock_guard lock(m_Mutex);
            m_Request.completionFence.emplace(std::move(completionFence));
            if (m_Request.phase.load(std::memory_order_acquire) != WorkerPhase::StopRequested) {
                m_Request.phase = WorkerPhase::AwaitingGpuCompletion;
            }
            return;
        }

        TF3D_LOG_ERROR("Failed to create OpenGL completion fence for generation worker '{}'", m_Config.name);
        TF3D_PROFILE_DRAIN_GPU();
        {
            std::lock_guard lock(m_Mutex);
            m_Request.completedRequestId = m_Request.activeRequestId.load(std::memory_order_acquire);
            m_Request.activeRequestId    = 0;
            if (m_Request.phase.load(std::memory_order_acquire) != WorkerPhase::StopRequested) {
                m_Request.phase = WorkerPhase::CompletionReady;
            }
        }
        m_Condition.notify_all();
    }

} // namespace tf3d::generators
