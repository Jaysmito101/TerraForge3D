#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class PerformanceMonitor
{
public:
    enum class CaptureMode : uint8_t {
        Off,
        Cpu,
        Full,
    };

    enum class Domain : uint8_t {
        Cpu,
        Gpu,
        Generation,
        Renderer,
        Worker,
        Job,
        Io,
        Resource,
        Ui,
        Present,
        Wait,
        Unknown,
    };

    enum class EventKind : uint8_t {
        Span,
        Counter,
        Instant,
        FlowBegin,
        FlowStep,
        FlowEnd,
    };

    enum EventFlags : uint32_t {
        EventComplete       = 1u << 0,
        EventCrossFrame     = 1u << 1,
        EventMismatch       = 1u << 2,
        EventDropped        = 1u << 3,
        EventGpuUnavailable = 1u << 4,
        EventGpuDelayed     = 1u << 5,
        EventStale          = 1u << 6,
        EventSkipped        = 1u << 7,
    };

    struct EventSnapshot {
        uint64_t eventId       = 0;
        uint64_t parentEventId = 0;
        uint64_t frameId       = 0;
        uint64_t flowId        = 0;
        uint64_t nameId        = 0;
        uint64_t threadId      = 0;
        uint64_t contextId     = 0;
        uint64_t startTicks    = 0;
        uint64_t value0        = 0;
        uint64_t value1        = 0;
        uint64_t value2        = 0;
        uint32_t flags         = 0;
        uint32_t depth         = 0;
        uint8_t valueCount     = 0;
        Domain domain          = Domain::Unknown;
        EventKind kind         = EventKind::Span;
        double durationMs      = 0.0;
        double cpuDurationMs   = 0.0;
        double gpuDurationMs   = 0.0;
        double startOffsetMs   = 0.0;
        double value           = 0.0;
        std::string key;
        std::string parentKey;
        std::string step;
        std::string threadName;
    };

    struct FrameSnapshot {
        uint64_t index      = 0;
        uint64_t startTicks = 0;
        double durationMs   = 0.0;
        bool complete       = false;
        std::vector<EventSnapshot> events;
    };

    struct MetadataEntry {
        std::string key;
        std::string value;
    };

    struct Snapshot {
        std::vector<FrameSnapshot> capturedFrames;
        std::vector<EventSnapshot> unframedEvents;
        std::vector<MetadataEntry> metadata;
        double capturedDurationMs        = 0.0;
        std::size_t capturedEventCount   = 0;
        uint64_t droppedEventCount       = 0;
        uint64_t incompleteEventCount    = 0;
        uint64_t pendingGpuQueryCount    = 0;
        uint64_t droppedGpuQueryCount    = 0;
        uint64_t profilerOverheadNs      = 0;
        uint64_t profilerOverheadSamples = 0;
        bool isCapturing                 = false;
        CaptureMode mode                 = CaptureMode::Off;
    };

    class Scope
    {
    public:
        Scope() = default;

        Scope(const Scope &)            = delete;
        Scope &operator=(const Scope &) = delete;

        Scope(Scope &&other) noexcept
            : m_Monitor(other.m_Monitor), m_Recorder(other.m_Recorder), m_EventId(other.m_EventId)
        {
            other.m_Monitor  = nullptr;
            other.m_Recorder = nullptr;
            other.m_EventId  = 0;
        }

        Scope &operator=(Scope &&other) noexcept
        {
            if (this == &other)
                return *this;
            End();
            m_Monitor        = other.m_Monitor;
            m_Recorder       = other.m_Recorder;
            m_EventId        = other.m_EventId;
            other.m_Monitor  = nullptr;
            other.m_Recorder = nullptr;
            other.m_EventId  = 0;
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
        friend class PerformanceMonitor;

        Scope(PerformanceMonitor *monitor, void *recorder, uint64_t eventId)
            : m_Monitor(monitor), m_Recorder(recorder), m_EventId(eventId)
        {
        }

        PerformanceMonitor *m_Monitor = nullptr;
        void *m_Recorder              = nullptr;
        uint64_t m_EventId            = 0;
    };

    class GpuScope
    {
    public:
        GpuScope() = default;

        GpuScope(const GpuScope &)            = delete;
        GpuScope &operator=(const GpuScope &) = delete;

        GpuScope(GpuScope &&other) noexcept
            : m_Monitor(other.m_Monitor), m_State(other.m_State), m_Slot(other.m_Slot), m_Generation(other.m_Generation)
        {
            other.m_Monitor    = nullptr;
            other.m_State      = nullptr;
            other.m_Slot       = 0;
            other.m_Generation = 0;
        }

        GpuScope &operator=(GpuScope &&other) noexcept
        {
            if (this == &other)
                return *this;
            End();
            m_Monitor          = other.m_Monitor;
            m_State            = other.m_State;
            m_Slot             = other.m_Slot;
            m_Generation       = other.m_Generation;
            other.m_Monitor    = nullptr;
            other.m_State      = nullptr;
            other.m_Slot       = 0;
            other.m_Generation = 0;
            return *this;
        }

        ~GpuScope()
        {
            End();
        }

        void End();

        bool IsActive() const
        {
            return m_Monitor != nullptr && m_State != nullptr;
        }

    private:
        friend class PerformanceMonitor;

        GpuScope(PerformanceMonitor *monitor, void *state, uint32_t slot, uint64_t generation)
            : m_Monitor(monitor), m_State(state), m_Slot(slot), m_Generation(generation)
        {
        }

        PerformanceMonitor *m_Monitor = nullptr;
        void *m_State                 = nullptr;
        uint32_t m_Slot               = 0;
        uint64_t m_Generation         = 0;
    };

    static PerformanceMonitor &Get();

    PerformanceMonitor(const PerformanceMonitor &)            = delete;
    PerformanceMonitor &operator=(const PerformanceMonitor &) = delete;

    void BeginFrame();
    void EndFrame();

    Scope BeginScope(std::string_view key, Domain domain = Domain::Cpu, uint64_t flowId = 0);

    template <typename KeyFactory>
    Scope BeginScopeLazy(KeyFactory &&factory, Domain domain = Domain::Cpu, uint64_t flowId = 0)
    {
        if (GetCaptureMode() == CaptureMode::Off)
            return {};
        return BeginScope(std::forward<KeyFactory>(factory)(), domain, flowId);
    }
    uint64_t BeginEvent(std::string_view key, Domain domain = Domain::Cpu, uint64_t flowId = 0);
    void EndEvent(uint64_t eventId);

    GpuScope BeginGpuScope(std::string_view key, Domain domain = Domain::Gpu, uint64_t flowId = 0);

    template <typename KeyFactory>
    GpuScope BeginGpuScopeLazy(KeyFactory &&factory, Domain domain = Domain::Gpu, uint64_t flowId = 0)
    {
        if (GetCaptureMode() != CaptureMode::Full)
            return {};
        return BeginGpuScope(std::forward<KeyFactory>(factory)(), domain, flowId);
    }

    void PollGpuQueries();

    void SetCurrentThreadName(std::string_view name);
    uint64_t CurrentFrameId() const;
    uint64_t NewFlowId();

    void RecordCounter(std::string_view key, double value, Domain domain = Domain::Cpu, uint64_t flowId = 0);
    void RecordValue(std::string_view key, uint64_t value0, uint64_t value1 = 0, uint64_t value2 = 0,
                     Domain domain = Domain::Cpu, uint64_t flowId = 0);
    void RecordInstant(std::string_view key, Domain domain = Domain::Cpu, uint64_t flowId = 0);
    void RecordFlowBegin(std::string_view key, uint64_t flowId, Domain domain = Domain::Cpu);
    void RecordFlowStep(std::string_view key, uint64_t flowId, std::string_view step,
                        Domain domain = Domain::Cpu);
    void RecordFlowEnd(std::string_view key, uint64_t flowId, Domain domain = Domain::Cpu);
    void SetMetadata(std::string_view key, std::string_view value);

    Snapshot CaptureSnapshot() const;
    void StartCapture(CaptureMode mode = CaptureMode::Full);
    void StopCapture();
    void ClearCapture();
    bool IsCapturing() const;
    CaptureMode GetCaptureMode() const;

    bool ExportChromeTrace(const std::string &path) const;

    static const char *DomainName(Domain domain);
    static const char *CaptureModeName(CaptureMode mode);

    void RenderUI(bool *windowOpen);

private:
    struct Recorder;
    struct GpuContext;
    struct FrameData;

    PerformanceMonitor();
    ~PerformanceMonitor();

    Scope BeginScopeInternal(std::string_view key, Domain domain, uint64_t flowId);
    uint64_t BeginEventInternal(std::string_view key, Domain domain, uint64_t flowId, void **recorderOut);
    void EndEventInternal(void *recorder, uint64_t eventId);
    void EndGpuScope(void *state, uint32_t slot, uint64_t generation);
    Recorder &GetThreadRecorder();
    void RegisterRecorder(Recorder *recorder);
    void CollectCompletedEvents();
    void EnsureFrame(uint64_t frameId, uint64_t startTicks, bool complete, double durationMs);
    void AppendEvent(const EventSnapshot &event);
    bool IsCaptureGenerationActive(uint64_t generation) const;

    std::atomic<CaptureMode> m_CaptureMode{CaptureMode::Off};
    std::atomic<CaptureMode> m_LastCaptureMode{CaptureMode::Off};
    std::atomic<uint64_t> m_CaptureGeneration{1};
    std::atomic<uint64_t> m_CurrentFrameId{0};
    std::atomic<uint64_t> m_CurrentFrameStartTicks{0};
    std::atomic<uint64_t> m_NextFrameId{0};
    std::atomic<uint64_t> m_NextEventId{1};
    std::atomic<uint64_t> m_NextFlowId{1};

    std::atomic<uint64_t> m_DroppedEventCount{0};
    std::atomic<uint64_t> m_IncompleteEventCount{0};
    std::atomic<uint64_t> m_PendingGpuQueryCount{0};
    std::atomic<uint64_t> m_DroppedGpuQueryCount{0};
    std::atomic<uint64_t> m_ProfilerOverheadNs{0};
    std::atomic<uint64_t> m_ProfilerOverheadSamples{0};

    struct Storage;
    std::unique_ptr<Storage> m_Storage;
};

#define TF3D_PROFILE_DETAIL_JOIN_IMPL(left, right) left##right
#define TF3D_PROFILE_DETAIL_JOIN(left, right)      TF3D_PROFILE_DETAIL_JOIN_IMPL(left, right)

#if !defined(TF3D_PROFILER_ENABLED) || TF3D_PROFILER_ENABLED

#define TF3D_PROFILE_SCOPE(key) \
    auto TF3D_PROFILE_DETAIL_JOIN(_tf3dProfileScope_, __LINE__) = ::PerformanceMonitor::Get().BeginScope(key)

#define TF3D_PROFILE_BEGIN(variable, key) \
    auto variable = ::PerformanceMonitor::Get().BeginScope(key)

#define TF3D_PROFILE_SCOPE_DOMAIN(key, domain) \
    auto TF3D_PROFILE_DETAIL_JOIN(_tf3dProfileScope_, __LINE__) = ::PerformanceMonitor::Get().BeginScope(key, domain)

#define TF3D_PROFILE_SCOPE_FLOW(key, domain, flowId) \
    auto TF3D_PROFILE_DETAIL_JOIN(_tf3dProfileScope_, __LINE__) = ::PerformanceMonitor::Get().BeginScope(key, domain, flowId)

#define TF3D_PROFILE_SCOPE_LAZY(keyExpression) \
    auto TF3D_PROFILE_DETAIL_JOIN(_tf3dProfileScope_, __LINE__) = ::PerformanceMonitor::Get().BeginScopeLazy([&]() { return (keyExpression); })

#define TF3D_PROFILE_SCOPE_LAZY_DOMAIN(keyExpression, domain) \
    auto TF3D_PROFILE_DETAIL_JOIN(_tf3dProfileScope_, __LINE__) = ::PerformanceMonitor::Get().BeginScopeLazy([&]() { return (keyExpression); }, domain)

#define TF3D_PROFILE_BEGIN_LAZY_DOMAIN(variable, keyExpression, domain) \
    auto variable = ::PerformanceMonitor::Get().BeginScopeLazy([&]() { return (keyExpression); }, domain)

#define TF3D_PROFILE_BEGIN_LAZY_DOMAIN_FLOW(variable, keyExpression, domain, flowId) \
    auto variable = ::PerformanceMonitor::Get().BeginScopeLazy([&]() { return (keyExpression); }, domain, flowId)

#define TF3D_PROFILE_END(variable)      (variable).End()
#define TF3D_PROFILE_EVENT_BEGIN(key)   ::PerformanceMonitor::Get().BeginEvent(key)
#define TF3D_PROFILE_EVENT_END(eventId) ::PerformanceMonitor::Get().EndEvent(eventId)
#if !defined(TF3D_PROFILER_GPU) || TF3D_PROFILER_GPU
#define TF3D_PROFILE_GPU_SCOPE(key) \
    auto TF3D_PROFILE_DETAIL_JOIN(_tf3dGpuProfileScope_, __LINE__) = ::PerformanceMonitor::Get().BeginGpuScopeLazy([&]() { return (key); })
#define TF3D_PROFILE_GPU_SCOPE_DOMAIN(key, domain) \
    auto TF3D_PROFILE_DETAIL_JOIN(_tf3dGpuProfileScope_, __LINE__) = ::PerformanceMonitor::Get().BeginGpuScopeLazy([&]() { return (key); }, domain)
#define TF3D_PROFILE_GPU_SCOPE_FLOW(key, domain, flowId) \
    auto TF3D_PROFILE_DETAIL_JOIN(_tf3dGpuProfileScope_, __LINE__) = ::PerformanceMonitor::Get().BeginGpuScopeLazy([&]() { return (key); }, domain, flowId)
#define TF3D_PROFILE_BEGIN_GPU_LAZY_DOMAIN_FLOW(variable, keyExpression, domain, flowId) \
    auto variable = ::PerformanceMonitor::Get().BeginGpuScopeLazy([&]() { return (keyExpression); }, domain, flowId)
#else
#define TF3D_PROFILE_GPU_SCOPE(key) \
    do {                            \
        (void)sizeof(key);          \
    } while (false)
#define TF3D_PROFILE_GPU_SCOPE_DOMAIN(key, domain) \
    do {                                           \
        (void)sizeof(key);                         \
        (void)sizeof(domain);                      \
    } while (false)
#define TF3D_PROFILE_GPU_SCOPE_FLOW(key, domain, flowId) \
    do {                                                 \
        (void)sizeof(key);                               \
        (void)sizeof(domain);                            \
        (void)sizeof(flowId);                            \
    } while (false)
#define TF3D_PROFILE_BEGIN_GPU_LAZY_DOMAIN_FLOW(variable, keyExpression, domain, flowId) \
    [[maybe_unused]] ::PerformanceMonitor::GpuScope variable = ((void)sizeof(keyExpression), (void)sizeof(domain), (void)sizeof(flowId), ::PerformanceMonitor::GpuScope{})
#endif
#define TF3D_PROFILE_COUNTER(key, value) \
    ::PerformanceMonitor::Get().RecordCounter(key, value)
#define TF3D_PROFILE_COUNTER_DOMAIN(key, value, domain) \
    ::PerformanceMonitor::Get().RecordCounter(key, value, domain)
#define TF3D_PROFILE_COUNTER_DOMAIN_FLOW(key, value, domain, flowId) \
    ::PerformanceMonitor::Get().RecordCounter(key, value, domain, flowId)
#define TF3D_PROFILE_VALUE(key, value0, value1, value2) \
    ::PerformanceMonitor::Get().RecordValue(key, value0, value1, value2)
#define TF3D_PROFILE_VALUE_DOMAIN(key, value0, value1, value2, domain) \
    ::PerformanceMonitor::Get().RecordValue(key, value0, value1, value2, domain)
#define TF3D_PROFILE_VALUE_DOMAIN_FLOW(key, value0, value1, value2, domain, flowId) \
    ::PerformanceMonitor::Get().RecordValue(key, value0, value1, value2, domain, flowId)
#define TF3D_PROFILE_INSTANT(key) \
    ::PerformanceMonitor::Get().RecordInstant(key)
#define TF3D_PROFILE_INSTANT_DOMAIN(key, domain) \
    ::PerformanceMonitor::Get().RecordInstant(key, domain)
#define TF3D_PROFILE_INSTANT_DOMAIN_FLOW(key, domain, flowId) \
    ::PerformanceMonitor::Get().RecordInstant(key, domain, flowId)
#define TF3D_PROFILE_THREAD_NAME(name) \
    ::PerformanceMonitor::Get().SetCurrentThreadName(name)
#define TF3D_PROFILE_CAPTURE_ACTIVE() \
    ::PerformanceMonitor::Get().IsCapturing()
#define TF3D_PROFILE_CAPTURE_MODE() \
    ::PerformanceMonitor::Get().GetCaptureMode()
#define TF3D_PROFILE_CURRENT_FRAME_ID() \
    ::PerformanceMonitor::Get().CurrentFrameId()
#define TF3D_PROFILE_NEW_FLOW_ID() \
    ::PerformanceMonitor::Get().NewFlowId()
#define TF3D_PROFILE_SET_METADATA(key, value) \
    ::PerformanceMonitor::Get().SetMetadata(key, value)
#define TF3D_PROFILE_FRAME_BEGIN() \
    ::PerformanceMonitor::Get().BeginFrame()
#define TF3D_PROFILE_FRAME_END() \
    ::PerformanceMonitor::Get().EndFrame()
#define TF3D_PROFILE_RENDER_UI(windowOpen) \
    ::PerformanceMonitor::Get().RenderUI(windowOpen)

#else

#define TF3D_PROFILE_SCOPE(key) \
    do {                        \
        (void)sizeof(key);      \
    } while (false)
#define TF3D_PROFILE_BEGIN(variable, key) \
    [[maybe_unused]] ::PerformanceMonitor::Scope variable = ((void)sizeof(key), ::PerformanceMonitor::Scope{})
#define TF3D_PROFILE_SCOPE_DOMAIN(key, domain) \
    do {                                       \
        (void)sizeof(key);                     \
        (void)sizeof(domain);                  \
    } while (false)
#define TF3D_PROFILE_SCOPE_FLOW(key, domain, flowId) \
    do {                                             \
        (void)sizeof(key);                           \
        (void)sizeof(domain);                        \
        (void)sizeof(flowId);                        \
    } while (false)
#define TF3D_PROFILE_SCOPE_LAZY(keyExpression) \
    do {                                       \
        (void)sizeof(keyExpression);           \
    } while (false)
#define TF3D_PROFILE_SCOPE_LAZY_DOMAIN(keyExpression, domain) \
    do {                                                      \
        (void)sizeof(keyExpression);                          \
        (void)sizeof(domain);                                 \
    } while (false)
#define TF3D_PROFILE_BEGIN_LAZY_DOMAIN(variable, keyExpression, domain) \
    [[maybe_unused]] ::PerformanceMonitor::Scope variable = ((void)sizeof(keyExpression), (void)sizeof(domain), ::PerformanceMonitor::Scope{})
#define TF3D_PROFILE_BEGIN_LAZY_DOMAIN_FLOW(variable, keyExpression, domain, flowId) \
    [[maybe_unused]] ::PerformanceMonitor::Scope variable = ((void)sizeof(keyExpression), (void)sizeof(domain), (void)sizeof(flowId), ::PerformanceMonitor::Scope{})
#define TF3D_PROFILE_END(variable) \
    do {                           \
        (void)sizeof(variable);    \
    } while (false)
#define TF3D_PROFILE_EVENT_BEGIN(key) ((void)sizeof(key), uint64_t(0))
#define TF3D_PROFILE_EVENT_END(eventId) \
    do {                                \
        (void)sizeof(eventId);          \
    } while (false)
#define TF3D_PROFILE_GPU_SCOPE(key) \
    do {                            \
        (void)sizeof(key);          \
    } while (false)
#define TF3D_PROFILE_GPU_SCOPE_DOMAIN(key, domain) \
    do {                                           \
        (void)sizeof(key);                         \
        (void)sizeof(domain);                      \
    } while (false)
#define TF3D_PROFILE_GPU_SCOPE_FLOW(key, domain, flowId) \
    do {                                                 \
        (void)sizeof(key);                               \
        (void)sizeof(domain);                            \
        (void)sizeof(flowId);                            \
    } while (false)
#define TF3D_PROFILE_BEGIN_GPU_LAZY_DOMAIN_FLOW(variable, keyExpression, domain, flowId) \
    [[maybe_unused]] ::PerformanceMonitor::GpuScope variable = ((void)sizeof(keyExpression), (void)sizeof(domain), (void)sizeof(flowId), ::PerformanceMonitor::GpuScope{})
#define TF3D_PROFILE_COUNTER(key, value) \
    do {                                 \
        (void)sizeof(key);               \
        (void)sizeof(value);             \
    } while (false)
#define TF3D_PROFILE_COUNTER_DOMAIN(key, value, domain) \
    do {                                                \
        (void)sizeof(key);                              \
        (void)sizeof(value);                            \
        (void)sizeof(domain);                           \
    } while (false)
#define TF3D_PROFILE_COUNTER_DOMAIN_FLOW(key, value, domain, flowId) \
    do {                                                             \
        (void)sizeof(key);                                           \
        (void)sizeof(value);                                         \
        (void)sizeof(domain);                                        \
        (void)sizeof(flowId);                                        \
    } while (false)
#define TF3D_PROFILE_VALUE(key, value0, value1, value2) \
    do {                                                \
        (void)sizeof(key);                              \
        (void)sizeof(value0);                           \
        (void)sizeof(value1);                           \
        (void)sizeof(value2);                           \
    } while (false)
#define TF3D_PROFILE_VALUE_DOMAIN(key, value0, value1, value2, domain) \
    do {                                                               \
        (void)sizeof(key);                                             \
        (void)sizeof(value0);                                          \
        (void)sizeof(value1);                                          \
        (void)sizeof(value2);                                          \
        (void)sizeof(domain);                                          \
    } while (false)
#define TF3D_PROFILE_VALUE_DOMAIN_FLOW(key, value0, value1, value2, domain, flowId) \
    do {                                                                            \
        (void)sizeof(key);                                                          \
        (void)sizeof(value0);                                                       \
        (void)sizeof(value1);                                                       \
        (void)sizeof(value2);                                                       \
        (void)sizeof(domain);                                                       \
        (void)sizeof(flowId);                                                       \
    } while (false)
#define TF3D_PROFILE_INSTANT(key) \
    do {                          \
        (void)sizeof(key);        \
    } while (false)
#define TF3D_PROFILE_INSTANT_DOMAIN(key, domain) \
    do {                                         \
        (void)sizeof(key);                       \
        (void)sizeof(domain);                    \
    } while (false)
#define TF3D_PROFILE_INSTANT_DOMAIN_FLOW(key, domain, flowId) \
    do {                                                      \
        (void)sizeof(key);                                    \
        (void)sizeof(domain);                                 \
        (void)sizeof(flowId);                                 \
    } while (false)
#define TF3D_PROFILE_THREAD_NAME(name) \
    do {                               \
        (void)sizeof(name);            \
    } while (false)
#define TF3D_PROFILE_CAPTURE_ACTIVE() false
#define TF3D_PROFILE_CAPTURE_MODE()   ::PerformanceMonitor::CaptureMode::Off
#define TF3D_PROFILE_CURRENT_FRAME_ID() uint64_t(0)
#define TF3D_PROFILE_NEW_FLOW_ID()    uint64_t(0)
#define TF3D_PROFILE_SET_METADATA(key, value) \
    do {                                      \
        (void)sizeof(key);                    \
        (void)sizeof(value);                  \
    } while (false)
#define TF3D_PROFILE_FRAME_BEGIN() \
    do {                                  \
    } while (false)
#define TF3D_PROFILE_FRAME_END() \
    do {                                \
    } while (false)
#define TF3D_PROFILE_RENDER_UI(windowOpen) \
    do {                                      \
        (void)sizeof(windowOpen);              \
    } while (false)

#endif

#if (!defined(TF3D_PROFILER_ENABLED) || TF3D_PROFILER_ENABLED) && \
    (!defined(TF3D_PROFILER_GPU) || TF3D_PROFILER_GPU)
#define TF3D_PROFILE_POLL_GPU() ::PerformanceMonitor::Get().PollGpuQueries()
#else
#define TF3D_PROFILE_POLL_GPU() \
    do {                        \
    } while (false)
#endif

#if !defined(TF3D_PROFILER_ENABLED) || TF3D_PROFILER_ENABLED
#define TF3D_PROFILE_FLOW_BEGIN_DOMAIN(key, flowId, domain) \
    ::PerformanceMonitor::Get().RecordFlowBegin(key, flowId, domain)
#define TF3D_PROFILE_FLOW_END_DOMAIN(key, flowId, domain) \
    ::PerformanceMonitor::Get().RecordFlowEnd(key, flowId, domain)
#define TF3D_PROFILE_FLOW_BEGIN(key, flowId) \
    ::PerformanceMonitor::Get().RecordFlowBegin(key, flowId)
#define TF3D_PROFILE_FLOW_STEP_DOMAIN(key, flowId, step, domain) \
    ::PerformanceMonitor::Get().RecordFlowStep(key, flowId, step, domain)
#define TF3D_PROFILE_FLOW_STEP(key, flowId, step) \
    ::PerformanceMonitor::Get().RecordFlowStep(key, flowId, step)
#define TF3D_PROFILE_FLOW_END(key, flowId) \
    ::PerformanceMonitor::Get().RecordFlowEnd(key, flowId)
#else
#define TF3D_PROFILE_FLOW_BEGIN(key, flowId) \
    do {                                     \
    } while (false)
#define TF3D_PROFILE_FLOW_END(key, flowId) \
    do {                                   \
    } while (false)
#define TF3D_PROFILE_FLOW_BEGIN_DOMAIN(key, flowId, domain) \
    do {                                                    \
    } while (false)
#define TF3D_PROFILE_FLOW_STEP_DOMAIN(key, flowId, step, domain) \
    do {                                                         \
    } while (false)
#define TF3D_PROFILE_FLOW_STEP(key, flowId, step) \
    do {                                          \
    } while (false)
#define TF3D_PROFILE_FLOW_END_DOMAIN(key, flowId, domain) \
    do {                                                  \
    } while (false)
#endif
