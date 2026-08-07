#include "Generators/GenerationWorker.h"

#include "Profiler.h"
#include "Utils/Utils.h"

#include <GLFW/glfw3.h>

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

    bool GenerationWorker::Request(bool force, uint64_t *requestIdOut)
    {
        if (!HasContext()) {
            return false;
        }

        bool newRequest    = false;
        uint64_t requestId = 0;
        {
            std::lock_guard lock(m_Mutex);
            if (!m_RequestPending.load(std::memory_order_acquire)) {
                m_LastRequestId = PerformanceMonitor::Get().NewFlowId();
                newRequest      = true;
            }
            requestId        = m_LastRequestId.load(std::memory_order_acquire);
            m_RequestPending = true;
            m_ForceRequested = m_ForceRequested || force;
        }
        if (requestIdOut != nullptr)
            *requestIdOut = requestId;
        if (TF3D_PROFILE_CAPTURE_ACTIVE()) {
            if (newRequest) {
                const std::string requestKey = m_ProfilePrefix + "/request";
                const std::string forceKey   = m_ProfilePrefix + "/request/force";
                TF3D_PROFILE_FLOW_BEGIN_DOMAIN(requestKey, requestId, PerformanceMonitor::Domain::Generation);
                TF3D_PROFILE_FLOW_STEP_DOMAIN(requestKey, requestId, "queued", PerformanceMonitor::Domain::Generation);
                TF3D_PROFILE_VALUE_DOMAIN_FLOW(forceKey, force ? 1 : 0, 0, 0,
                                               PerformanceMonitor::Domain::Generation, requestId);
            } else {
                const std::string requestKey   = m_ProfilePrefix + "/request";
                const std::string coalescedKey = m_ProfilePrefix + "/request/coalesced";
                TF3D_PROFILE_FLOW_STEP_DOMAIN(requestKey, requestId, "coalesced", PerformanceMonitor::Domain::Generation);
                TF3D_PROFILE_COUNTER_DOMAIN_FLOW(coalescedKey, 1.0, PerformanceMonitor::Domain::Generation, requestId);
            }
        }
        m_Condition.notify_one();
        return true;
    }

    void GenerationWorker::WaitForIdle()
    {
        TF3D_PROFILE_BEGIN_LAZY_DOMAIN(waitScope, m_ProfilePrefix + "/queue-wait", PerformanceMonitor::Domain::Wait);
        std::unique_lock lock(m_Mutex);
        m_Condition.wait(lock, [this] {
            return !m_Running.load(std::memory_order_acquire) &&
                   !m_RequestPending.load(std::memory_order_acquire);
        });
        waitScope.End();
    }

    bool GenerationWorker::ConsumeCompleted()
    {
        return m_Completed.exchange(false, std::memory_order_acq_rel);
    }

    void GenerationWorker::Run()
    {
        glfwMakeContextCurrent(m_Window);
        TF3D_PROFILE_THREAD_NAME(m_Name);
        while (true) {
            bool force         = false;
            uint64_t requestId = 0;
            {
                TF3D_PROFILE_BEGIN_LAZY_DOMAIN(queueWaitScope, m_ProfilePrefix + "/queue-wait", PerformanceMonitor::Domain::Wait);
                std::unique_lock lock(m_Mutex);
                m_Condition.wait(lock, [this] {
                    return m_StopRequested.load(std::memory_order_acquire) ||
                           m_RequestPending.load(std::memory_order_acquire);
                });
                if (m_StopRequested.load(std::memory_order_acquire))
                    break;

                force            = m_ForceRequested;
                requestId        = m_LastRequestId.load(std::memory_order_acquire);
                m_RequestPending = false;
                m_ForceRequested = false;
                m_Running        = true;
                queueWaitScope.End();
            }

            m_ActiveRequestId        = requestId;
            const bool captureActive = TF3D_PROFILE_CAPTURE_ACTIVE();
            if (captureActive) {
                const std::string workerKey = m_ProfilePrefix + "/worker";
                TF3D_PROFILE_FLOW_BEGIN_DOMAIN(workerKey, requestId, PerformanceMonitor::Domain::Worker);
                TF3D_PROFILE_FLOW_STEP_DOMAIN(workerKey, requestId, "started", PerformanceMonitor::Domain::Worker);
            }
            {
                TF3D_PROFILE_BEGIN_LAZY_DOMAIN_FLOW(executeScope, m_ProfilePrefix + "/worker/execute",
                                                    PerformanceMonitor::Domain::Worker, requestId);
                TF3D_PROFILE_BEGIN_GPU_LAZY_DOMAIN_FLOW(gpuScope, m_ProfilePrefix + "/worker/gpu",
                                                        PerformanceMonitor::Domain::Gpu, requestId);
                if (captureActive) {
                    const std::string requestKey = m_ProfilePrefix + "/worker/request";
                    TF3D_PROFILE_VALUE_DOMAIN_FLOW(requestKey, requestId, force ? 1 : 0, 0,
                                                   PerformanceMonitor::Domain::Worker, requestId);
                }
                if (m_Callback)
                    m_Callback(force);
            }
            if (captureActive) {
                const std::string workerKey = m_ProfilePrefix + "/worker";
                TF3D_PROFILE_FLOW_STEP_DOMAIN(workerKey, requestId, "callback-complete", PerformanceMonitor::Domain::Worker);
            }

            {
                TF3D_PROFILE_BEGIN_LAZY_DOMAIN_FLOW(synchronizeScope, m_ProfilePrefix + "/worker/synchronize",
                                                    PerformanceMonitor::Domain::Wait, requestId);
                if (captureActive) {
                    const std::string barrierKey = m_ProfilePrefix + "/barriers";
                    TF3D_PROFILE_COUNTER_DOMAIN_FLOW(barrierKey, 1.0, PerformanceMonitor::Domain::Wait, requestId);
                }
                glMemoryBarrier(GL_ALL_BARRIER_BITS);
                if (captureActive) {
                    const std::string finishKey = m_ProfilePrefix + "/gl-finish";
                    TF3D_PROFILE_INSTANT_DOMAIN_FLOW(finishKey, PerformanceMonitor::Domain::Wait, requestId);
                }
                glFinish();
            }
            TF3D_PROFILE_POLL_GPU();
            if (captureActive) {
                const std::string workerKey = m_ProfilePrefix + "/worker";
                TF3D_PROFILE_FLOW_STEP_DOMAIN(workerKey, requestId, "gpu-complete", PerformanceMonitor::Domain::Worker);
                TF3D_PROFILE_FLOW_END_DOMAIN(workerKey, requestId, PerformanceMonitor::Domain::Worker);
            }

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
