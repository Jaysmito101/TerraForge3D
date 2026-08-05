#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

class PerformanceMonitor
{
public:
    struct EventSnapshot {
        uint64_t eventId       = 0;
        uint64_t parentEventId = 0;
        std::string key;
        std::string parentKey;
        double durationMs    = 0.0;
        double startOffsetMs = 0.0;
        uint64_t threadId    = 0;
        std::string threadName;
        std::size_t depth = 0;
        std::vector<std::string> callStack;
    };

    struct FrameSnapshot {
        uint64_t index    = 0;
        double durationMs = 0.0;
        std::vector<EventSnapshot> events;
    };

    struct Snapshot {
        std::vector<FrameSnapshot> capturedFrames;
        double capturedDurationMs      = 0.0;
        std::size_t capturedEventCount = 0;
        bool isCapturing               = false;
    };

    class Scope
    {
    public:
        Scope() = default;
        Scope(PerformanceMonitor *monitor, uint64_t eventId)
            : m_Monitor(monitor), m_EventId(eventId)
        {
        }

        Scope(const Scope &)            = delete;
        Scope &operator=(const Scope &) = delete;

        Scope(Scope &&other) noexcept
            : m_Monitor(other.m_Monitor), m_EventId(other.m_EventId)
        {
            other.m_Monitor = nullptr;
            other.m_EventId = 0;
        }

        Scope &operator=(Scope &&other) noexcept
        {
            if (this == &other)
                return *this;
            End();
            m_Monitor       = other.m_Monitor;
            m_EventId       = other.m_EventId;
            other.m_Monitor = nullptr;
            other.m_EventId = 0;
            return *this;
        }

        ~Scope()
        {
            End();
        }

        void End();
        bool IsActive() const
        {
            return m_Monitor != nullptr && m_EventId != 0;
        }

    private:
        PerformanceMonitor *m_Monitor = nullptr;
        uint64_t m_EventId            = 0;
    };

    static PerformanceMonitor &Get();

    PerformanceMonitor(const PerformanceMonitor &)            = delete;
    PerformanceMonitor &operator=(const PerformanceMonitor &) = delete;

    void BeginFrame();
    void EndFrame();

    Scope BeginScope(std::string_view key);
    uint64_t BeginEvent(std::string_view key);
    void EndEvent(uint64_t eventId);
    void SetCurrentThreadName(std::string_view name);

    Snapshot CaptureSnapshot() const;
    void StartCapture();
    void StopCapture();
    void ClearCapture();

    void RenderUI();
    bool *IsWindowOpenPtr()
    {
        return &m_WindowOpen;
    }
    bool IsWindowOpen() const
    {
        return m_WindowOpen;
    }

private:
    using Clock = std::chrono::steady_clock;

    struct ActiveEvent {
        uint64_t parentEventId = 0;
        std::string key;
        std::string parentKey;
        Clock::time_point start;
        uint64_t threadId = 0;
        std::string threadName;
        std::size_t depth = 0;
        std::vector<std::string> callStack;
    };

    struct FrameData {
        uint64_t index    = 0;
        double durationMs = 0.0;
        std::vector<EventSnapshot> events;
    };

    PerformanceMonitor() = default;

    static uint64_t GetThreadId();

    mutable std::mutex m_Mutex;
    std::unordered_map<uint64_t, std::string> m_ThreadNames;
    std::unordered_map<uint64_t, ActiveEvent> m_ActiveEvents;
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_ThreadStacks;
    std::vector<FrameData> m_CapturedFrames;
    FrameData m_CurrentFrame;
    Clock::time_point m_FrameStart;
    uint64_t m_NextEventId = 1;
    uint64_t m_NextFrameId = 0;
    bool m_FrameOpen       = false;
    bool m_IsCapturing     = false;

    bool m_WindowOpen            = false;
    bool m_GroupCallTreeByThread = false;
};

#define TF3D_PROFILE_DETAIL_JOIN_IMPL(left, right) left##right
#define TF3D_PROFILE_DETAIL_JOIN(left, right)      TF3D_PROFILE_DETAIL_JOIN_IMPL(left, right)

#define TF3D_PROFILE_SCOPE(key) \
    auto TF3D_PROFILE_DETAIL_JOIN(_tf3dProfileScope_, __LINE__) = ::PerformanceMonitor::Get().BeginScope(key)

#define TF3D_PROFILE_BEGIN(variable, key) \
    auto variable = ::PerformanceMonitor::Get().BeginScope(key)

#define TF3D_PROFILE_END(variable)      (variable).End()

#define TF3D_PROFILE_EVENT_BEGIN(key)   ::PerformanceMonitor::Get().BeginEvent(key)
#define TF3D_PROFILE_EVENT_END(eventId) ::PerformanceMonitor::Get().EndEvent(eventId)

#define TF3D_PERF_SCOPE(key)            TF3D_PROFILE_SCOPE(key)
#define TF3D_PERF_BEGIN(variable, key)  TF3D_PROFILE_BEGIN(variable, key)
#define TF3D_PERF_END(variable)         TF3D_PROFILE_END(variable)
