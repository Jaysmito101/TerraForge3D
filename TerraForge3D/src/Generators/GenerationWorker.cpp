#include "Generators/GenerationWorker.h"

#include "Base/SyncFence.h"
#include "Profiler.h"
#include "Utils/Utils.h"

#include <GLFW/glfw3.h>

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
            tf3d::utils::ShowMessageBox(
                "The generation worker could not create a shared OpenGL context. Generation will run on the render thread and may temporarily block the UI (UI might feel laggy).",
                "Warning");
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

            requestId                    = NextUniqueId();
            m_Request.lastRequestId      = requestId;
            m_Request.phase              = WorkerPhase::RequestQueued;
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

    void GenerationWorker::WaitForIdle()
    {
        TF3D_PROFILE_SCOPE_LAZY_DOMAIN(m_Config.profilePrefix + "/wait-for-idle", PerformanceMonitor::Domain::Wait);
        std::unique_lock lock(m_Mutex);
        m_Condition.wait(lock, [this] {
            const WorkerPhase phase = m_Request.phase.load(std::memory_order_acquire);
            return phase == WorkerPhase::Idle || phase == WorkerPhase::CompletionReady || phase == WorkerPhase::Stopped;
        });
    }

    std::optional<uint64_t> GenerationWorker::TryConsumeCompleted()
    {
        std::optional<uint64_t> completedRequestId;
        {
            std::lock_guard lock(m_Mutex);
            if (m_Request.phase.load(std::memory_order_acquire) == WorkerPhase::CompletionReady) {
                completedRequestId           = m_Request.completedRequestId;
                m_Request.completedRequestId = 0;
                m_Request.phase              = WorkerPhase::Idle;
            }
        }
        if (!completedRequestId) {
            return std::nullopt;
        }
        m_Condition.notify_all();
        return completedRequestId;
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
            return phase == WorkerPhase::StopRequested || phase == WorkerPhase::RequestQueued;
        });

        if (m_Request.phase.load(std::memory_order_acquire) == WorkerPhase::StopRequested) {
            m_Request.phase           = WorkerPhase::Stopped;
            m_Request.activeRequestId = 0;
            return {WorkItemType::Stop};
        }

        const uint64_t requestId  = m_Request.lastRequestId.load(std::memory_order_acquire);
        m_Request.activeRequestId = requestId;
        m_Request.phase           = WorkerPhase::ExecutingRequest;
        return {WorkItemType::GenerateRequest, requestId};
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

        bool fenceCreated = false;
        {
            base::SyncFence completionFence;
            fenceCreated = static_cast<bool>(completionFence);
            {
                std::lock_guard lock(m_Mutex);
                if (fenceCreated && m_Request.phase.load(std::memory_order_acquire) != WorkerPhase::StopRequested) {
                    m_Request.phase = WorkerPhase::AwaitingGpuCompletion;
                }
            }

            if (fenceCreated) {
                const GLenum waitResult = completionFence.ClientWait(GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
                if (waitResult != GL_ALREADY_SIGNALED && waitResult != GL_CONDITION_SATISFIED) {
                    TF3D_LOG_ERROR("OpenGL completion fence failed for generation worker '{}'", m_Config.name);
                    glFinish();
                }
            }
        }

        if (!fenceCreated) {
            TF3D_LOG_ERROR("Failed to create OpenGL completion fence for generation worker '{}'", m_Config.name);
        }
        TF3D_PROFILE_DRAIN_GPU();
        {
            std::lock_guard lock(m_Mutex);
            m_Request.completedRequestId = requestId;
            m_Request.activeRequestId    = 0;
            if (m_Request.phase.load(std::memory_order_acquire) != WorkerPhase::StopRequested) {
                m_Request.phase = WorkerPhase::CompletionReady;
            }
        }
        m_Condition.notify_all();
    }

} // namespace tf3d::generators
