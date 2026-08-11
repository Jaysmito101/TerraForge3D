#include "Profiler.h"
#include "Utils/Utils.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <iomanip>
#include <limits>
#include <thread>
#include <unordered_map>

namespace
{
    constexpr std::size_t kEventCapacity    = 8192;
    constexpr std::size_t kNameCapacity     = 512;
    constexpr std::size_t kNameLength       = 128;
    constexpr std::size_t kMaxDepth         = 256;
    constexpr std::size_t kGpuQueryCapacity = 256;
    constexpr uint32_t kInvalidNameIndex    = std::numeric_limits<uint32_t>::max();

    using Clock = std::chrono::steady_clock;

    uint64_t NowTicks()
    {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count());
    }

    uint64_t ThreadId()
    {
        return static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    }

    uint64_t CurrentOpenGLContextId()
    {
        GLFWwindow *context = glfwGetCurrentContext();
        return context != nullptr ? reinterpret_cast<uint64_t>(context) : 0;
    }

    uint64_t HashName(std::string_view value)
    {
        uint64_t hash = 1469598103934665603ull;
        for (const unsigned char character : value) {
            hash ^= character;
            hash *= 1099511628211ull;
        }
        return hash == 0 ? 1 : hash;
    }

    std::string JsonEscape(std::string_view value)
    {
        std::string escaped;
        escaped.reserve(value.size() + 8);
        for (const char character : value) {
            switch (character) {
                case '"':
                    escaped += "\\\"";
                    break;
                case '\\':
                    escaped += "\\\\";
                    break;
                case '\n':
                    escaped += "\\n";
                    break;
                case '\r':
                    escaped += "\\r";
                    break;
                case '\t':
                    escaped += "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(character) < 0x20) {
                        escaped += "?";
                    } else {
                        escaped += character;
                    }
                    break;
            }
        }
        return escaped;
    }

    std::string EventFlagNames(uint32_t flags)
    {
        std::string names;
        const auto append = [&](uint32_t flag, std::string_view name) {
            if ((flags & flag) == 0)
                return;
            if (!names.empty())
                names += '|';
            names += name;
        };
        append(PerformanceMonitor::EventComplete, "complete");
        append(PerformanceMonitor::EventCrossFrame, "cross_frame");
        append(PerformanceMonitor::EventMismatch, "mismatch");
        append(PerformanceMonitor::EventDropped, "dropped");
        append(PerformanceMonitor::EventGpuUnavailable, "gpu_unavailable");
        append(PerformanceMonitor::EventGpuDelayed, "gpu_delayed");
        append(PerformanceMonitor::EventStale, "stale");
        append(PerformanceMonitor::EventSkipped, "skipped");
        return names;
    }

    bool IsGpuEvent(const PerformanceMonitor::EventSnapshot &event)
    {
        return event.domain == PerformanceMonitor::Domain::Gpu && event.contextId != 0;
    }

    uint64_t ChromeThreadId(const PerformanceMonitor::EventSnapshot &event)
    {
        // Keep GPU work on a stable synthetic track per OpenGL context. The high
        // range separates these IDs from recorder/thread IDs while staying within
        // a signed 64-bit Chrome trace ID.
        constexpr uint64_t kGpuTrackBit  = 1ull << 62;
        constexpr uint64_t kGpuTrackMask = kGpuTrackBit - 1;
        return IsGpuEvent(event) ? (kGpuTrackBit | (event.contextId & kGpuTrackMask)) : event.threadId;
    }

    std::string ChromeThreadName(const PerformanceMonitor::EventSnapshot &event)
    {
        if (IsGpuEvent(event))
            return "GPU Context " + std::to_string(event.contextId);
        if (!event.threadName.empty())
            return event.threadName;
        return "Thread " + std::to_string(event.threadId);
    }

    struct RawEvent {
        uint64_t eventId                   = 0;
        uint64_t parentEventId             = 0;
        uint64_t frameId                   = 0;
        uint64_t flowId                    = 0;
        uint64_t nameId                    = 0;
        uint64_t stepNameId                = 0;
        uint64_t threadId                  = 0;
        uint64_t contextId                 = 0;
        uint64_t captureGeneration         = 0;
        uint64_t startTicks                = 0;
        uint64_t durationTicks             = 0;
        uint64_t cpuDurationTicks          = 0;
        uint64_t gpuDurationTicks          = 0;
        uint64_t value0                    = 0;
        uint64_t value1                    = 0;
        uint64_t value2                    = 0;
        uint32_t nameIndex                 = kInvalidNameIndex;
        uint32_t stepNameIndex             = kInvalidNameIndex;
        uint32_t parentNameIndex           = kInvalidNameIndex;
        uint32_t flags                     = 0;
        uint32_t depth                     = 0;
        uint8_t valueCount                 = 0;
        double value                       = 0.0;
        PerformanceMonitor::Domain domain  = PerformanceMonitor::Domain::Unknown;
        PerformanceMonitor::EventKind kind = PerformanceMonitor::EventKind::Span;
    };

    struct NameEntry {
        uint64_t nameId = 0;
        std::array<char, kNameLength> text{};
        std::atomic_bool ready = false;
    };

    struct ActiveSpan {
        uint64_t eventId                  = 0;
        uint64_t parentEventId            = 0;
        uint64_t frameId                  = 0;
        uint64_t flowId                   = 0;
        uint64_t captureGeneration        = 0;
        uint64_t nameId                   = 0;
        uint64_t startTicks               = 0;
        uint32_t nameIndex                = kInvalidNameIndex;
        uint32_t parentNameIndex          = kInvalidNameIndex;
        uint32_t depth                    = 0;
        PerformanceMonitor::Domain domain = PerformanceMonitor::Domain::Cpu;
        bool active                       = false;
    };

    struct GpuQuerySlot {
        GLuint beginQuery                 = 0;
        GLuint endQuery                   = 0;
        uint64_t eventId                  = 0;
        uint64_t frameId                  = 0;
        uint64_t flowId                   = 0;
        uint64_t generation               = 0;
        uint64_t nameId                   = 0;
        uint64_t cpuStartTicks            = 0;
        uint64_t cpuEndTicks              = 0;
        uint32_t nameIndex                = kInvalidNameIndex;
        uint32_t flags                    = 0;
        PerformanceMonitor::Domain domain = PerformanceMonitor::Domain::Gpu;
        bool active                       = false;
        bool ended                        = false;
        bool invalid                      = false;
    };
} // namespace

struct PerformanceMonitor::GpuContext {
    uint64_t contextId = 0;
    std::array<GpuQuerySlot, kGpuQueryCapacity> slots{};
    std::array<GLuint, kGpuQueryCapacity> beginQueries{};
    std::array<GLuint, kGpuQueryCapacity> endQueries{};
    std::size_t nextSlot     = 0;
    int64_t gpuToCpuOffsetNs = 0;
    bool queriesCreated      = false;
    bool clockCalibrated     = false;
};

struct PerformanceMonitor::Recorder {
    explicit Recorder(PerformanceMonitor *ownerMonitor)
        : owner(ownerMonitor), threadId(ThreadId())
    {
        nameLookup.reserve(128);
    }

    uint32_t InternName(std::string_view key)
    {
        const uint64_t nameId = HashName(key);
        const auto iterator   = nameLookup.find(nameId);
        if (iterator != nameLookup.end())
            return iterator->second;

        if (nameCount >= kNameCapacity)
            return kInvalidNameIndex;

        const uint32_t index         = nameCount++;
        NameEntry &entry             = names[index];
        entry.nameId                 = nameId;
        const std::size_t copyLength = std::min(key.size(), kNameLength - 1);
        std::memcpy(entry.text.data(), key.data(), copyLength);
        entry.text[copyLength] = '\0';
        entry.ready.store(true, std::memory_order_release);
        nameLookup.emplace(nameId, index);
        return index;
    }

    void Append(const RawEvent &event)
    {
        const uint64_t readCursor = readSequence.load(std::memory_order_acquire);
        if (writeCursor - readCursor >= kEventCapacity) {
            owner->m_DroppedEventCount.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        events[writeCursor % kEventCapacity] = event;
        ++writeCursor;
        publishedSequence.store(writeCursor, std::memory_order_release);
    }

    PerformanceMonitor *owner = nullptr;
    uint64_t threadId         = 0;
    std::string threadName;

    std::array<NameEntry, kNameCapacity> names{};
    uint32_t nameCount = 0;
    std::unordered_map<uint64_t, uint32_t> nameLookup;

    std::array<ActiveSpan, kMaxDepth> stack{};
    uint32_t depth                 = 0;
    uint64_t overheadSampleCounter = 0;

    std::array<RawEvent, kEventCapacity> events{};
    uint64_t writeCursor = 0;
    std::atomic<uint64_t> publishedSequence{0};
    std::atomic<uint64_t> readSequence{0};

    std::deque<GpuContext> gpuContexts;
};

struct PerformanceMonitor::FrameData {
    FrameSnapshot snapshot;
};

struct PerformanceMonitor::Storage {
    mutable std::mutex registryMutex;
    mutable std::mutex dataMutex;
    std::vector<Recorder *> recorders;
    std::vector<FrameSnapshot> frames;
    std::unordered_map<uint64_t, std::size_t> frameIndices;
    std::vector<EventSnapshot> unframedEvents;
    std::vector<MetadataEntry> metadata;
};

std::string PerformanceMonitor::ResolveChildKey(std::string_view key)
{
    if (key.empty())
        return {};

    Recorder &recorder = GetThreadRecorder();
    if (recorder.depth == 0)
        return std::string(key);

    const ActiveSpan &parent = recorder.stack[recorder.depth - 1];
    if (!parent.active || parent.nameIndex == kInvalidNameIndex)
        return std::string(key);

    const NameEntry &parentName = recorder.names[parent.nameIndex];
    if (!parentName.ready.load(std::memory_order_acquire) || parentName.text[0] == '\0')
        return std::string(key);

    const std::string_view parentKey(parentName.text.data());
    std::string childKey;
    childKey.reserve(parentKey.size() + 1 + key.size());
    childKey.append(parentKey);
    childKey.push_back('/');
    childKey.append(key);
    return childKey;
}

PerformanceMonitor &PerformanceMonitor::Get()
{
    static PerformanceMonitor monitor;
    return monitor;
}

PerformanceMonitor::PerformanceMonitor()
    : m_Storage(std::make_unique<Storage>())
{
}

PerformanceMonitor::~PerformanceMonitor() = default;

const char *PerformanceMonitor::DomainName(Domain domain)
{
    switch (domain) {
        case Domain::Cpu:
            return "CPU";
        case Domain::Gpu:
            return "GPU";
        case Domain::Generation:
            return "Generation";
        case Domain::Renderer:
            return "Renderer";
        case Domain::Worker:
            return "Worker";
        case Domain::Job:
            return "Job";
        case Domain::Io:
            return "I/O";
        case Domain::Resource:
            return "Resource";
        case Domain::Ui:
            return "UI";
        case Domain::Present:
            return "Present";
        case Domain::Wait:
            return "Wait";
        default:
            return "Unknown";
    }
}

const char *PerformanceMonitor::CaptureModeName(CaptureMode mode)
{
    switch (mode) {
        case CaptureMode::Off:
            return "Off";
        case CaptureMode::Cpu:
            return "CPU";
        case CaptureMode::Full:
            return "Full";
        default:
            return "Unknown";
    }
}

PerformanceMonitor::Recorder &PerformanceMonitor::GetThreadRecorder()
{
    thread_local Recorder *recorder = nullptr;
    if (recorder == nullptr || recorder->owner != this) {
        recorder = new Recorder(this);
        RegisterRecorder(recorder);
    }
    return *recorder;
}

void PerformanceMonitor::RegisterRecorder(Recorder *recorder)
{
    std::lock_guard lock(m_Storage->registryMutex);
    m_Storage->recorders.push_back(recorder);
}

void PerformanceMonitor::SetCurrentThreadName(std::string_view name)
{
    Recorder &recorder = GetThreadRecorder();
    std::lock_guard lock(m_Storage->registryMutex);
    recorder.threadName = std::string(name);
}

uint64_t PerformanceMonitor::CurrentFrameId() const
{
    return m_CurrentFrameId.load(std::memory_order_acquire);
}

uint64_t PerformanceMonitor::NewFlowId()
{
    return NextUniqueId();
}

bool PerformanceMonitor::IsCapturing() const
{
    return m_CaptureMode.load(std::memory_order_acquire) != CaptureMode::Off;
}

PerformanceMonitor::CaptureMode PerformanceMonitor::GetCaptureMode() const
{
    return m_CaptureMode.load(std::memory_order_acquire);
}

bool PerformanceMonitor::IsCaptureGenerationActive(uint64_t generation) const
{
    if (generation != m_CaptureGeneration.load(std::memory_order_acquire))
        return false;

    const CaptureMode mode = GetCaptureMode();
    if (mode == CaptureMode::Full)
        return true;

    return mode == CaptureMode::Off && m_LastCaptureMode.load(std::memory_order_acquire) == CaptureMode::Full;
}

PerformanceMonitor::Scope PerformanceMonitor::BeginScope(std::string_view key, Domain domain, uint64_t flowId)
{
    return BeginScopeInternal(key, domain, flowId);
}

PerformanceMonitor::Scope PerformanceMonitor::BeginChildScope(std::string_view key, Domain domain, uint64_t flowId)
{
    if (GetCaptureMode() == CaptureMode::Off)
        return {};

    if (domain == Domain::Unknown) {
        Recorder &recorder = GetThreadRecorder();
        if (recorder.depth > 0) {
            const ActiveSpan &parent = recorder.stack[recorder.depth - 1];
            if (parent.active)
                domain = parent.domain;
        }
        if (domain == Domain::Unknown)
            domain = Domain::Cpu;
    }
    return BeginScope(ResolveChildKey(key), domain, flowId);
}

PerformanceMonitor::Scope PerformanceMonitor::BeginScopeInternal(std::string_view key, Domain domain, uint64_t flowId)
{
    void *recorder         = nullptr;
    const uint64_t eventId = BeginEventInternal(key, domain, flowId, &recorder);
    return Scope(this, recorder, eventId);
}

uint64_t PerformanceMonitor::BeginEvent(std::string_view key, Domain domain, uint64_t flowId)
{
    return BeginEventInternal(key, domain, flowId, nullptr);
}

uint64_t PerformanceMonitor::BeginEventInternal(std::string_view key, Domain domain, uint64_t flowId, void **recorderOut)
{
    if (key.empty() || GetCaptureMode() == CaptureMode::Off)
        return 0;

    Recorder &recorder           = GetThreadRecorder();
    const bool sampleOverhead    = (++recorder.overheadSampleCounter & 63u) == 0;
    const uint64_t overheadStart = sampleOverhead ? NowTicks() : 0;
    if (recorder.depth >= kMaxDepth) {
        m_IncompleteEventCount.fetch_add(1, std::memory_order_relaxed);
        if (sampleOverhead) {
            m_ProfilerOverheadNs.fetch_add(NowTicks() - overheadStart, std::memory_order_relaxed);
            m_ProfilerOverheadSamples.fetch_add(1, std::memory_order_relaxed);
        }
        return 0;
    }

    const uint64_t generation = m_CaptureGeneration.load(std::memory_order_acquire);
    const uint64_t eventId    = m_NextEventId.fetch_add(1, std::memory_order_relaxed);
    ActiveSpan span;
    span.eventId           = eventId;
    span.frameId           = m_CurrentFrameId.load(std::memory_order_acquire);
    span.flowId            = flowId;
    span.captureGeneration = generation;
    span.startTicks        = NowTicks();
    span.nameId            = HashName(key);
    span.nameIndex         = recorder.InternName(key);
    span.domain            = domain;
    span.depth             = recorder.depth;
    if (recorder.depth > 0) {
        const ActiveSpan &parent = recorder.stack[recorder.depth - 1];
        if (parent.active && parent.captureGeneration == generation) {
            span.parentEventId   = parent.eventId;
            span.parentNameIndex = parent.nameIndex;
            span.depth           = parent.depth + 1;
        }
    }
    span.active                      = true;
    recorder.stack[recorder.depth++] = span;
    if (recorder.depth == 0)
        recorder.depth = 1;

    if (recorder.depth > kMaxDepth)
        recorder.depth = static_cast<uint32_t>(kMaxDepth);

    if (recorder.depth > 0 && recorder.stack[recorder.depth - 1].eventId != eventId) {
        m_IncompleteEventCount.fetch_add(1, std::memory_order_relaxed);
        recorder.stack[recorder.depth - 1] = {};
        --recorder.depth;
        if (sampleOverhead) {
            m_ProfilerOverheadNs.fetch_add(NowTicks() - overheadStart, std::memory_order_relaxed);
            m_ProfilerOverheadSamples.fetch_add(1, std::memory_order_relaxed);
        }
        return 0;
    }

    if (sampleOverhead) {
        m_ProfilerOverheadNs.fetch_add(NowTicks() - overheadStart, std::memory_order_relaxed);
        m_ProfilerOverheadSamples.fetch_add(1, std::memory_order_relaxed);
    }
    if (recorderOut != nullptr)
        *recorderOut = &recorder;
    return eventId;
}

void PerformanceMonitor::Scope::End()
{
    if (m_Monitor == nullptr || m_EventId == 0)
        return;
    m_Monitor->EndEventInternal(m_Recorder, m_EventId);
    m_Monitor  = nullptr;
    m_Recorder = nullptr;
    m_EventId  = 0;
}

void PerformanceMonitor::EndEvent(uint64_t eventId)
{
    if (eventId == 0)
        return;
    EndEventInternal(&GetThreadRecorder(), eventId);
}

void PerformanceMonitor::EndEventInternal(void *recorderPointer, uint64_t eventId)
{
    if (recorderPointer == nullptr || eventId == 0)
        return;

    Recorder &recorder           = *static_cast<Recorder *>(recorderPointer);
    const bool sampleOverhead    = (++recorder.overheadSampleCounter & 63u) == 0;
    const uint64_t overheadStart = sampleOverhead ? NowTicks() : 0;
    while (recorder.depth > 0 && !recorder.stack[recorder.depth - 1].active)
        --recorder.depth;

    std::size_t stackIndex = recorder.depth;
    while (stackIndex > 0) {
        --stackIndex;
        if (recorder.stack[stackIndex].active && recorder.stack[stackIndex].eventId == eventId)
            break;
    }
    if (recorder.depth == 0 || stackIndex >= recorder.depth || recorder.stack[stackIndex].eventId != eventId) {
        if (sampleOverhead) {
            m_ProfilerOverheadNs.fetch_add(NowTicks() - overheadStart, std::memory_order_relaxed);
            m_ProfilerOverheadSamples.fetch_add(1, std::memory_order_relaxed);
        }
        return;
    }

    ActiveSpan span                   = recorder.stack[stackIndex];
    recorder.stack[stackIndex].active = false;
    const uint64_t endTicks           = NowTicks();
    const bool mismatch               = stackIndex + 1 != recorder.depth;
    const uint64_t durationTicks      = endTicks >= span.startTicks ? endTicks - span.startTicks : 0;
    const uint64_t currentFrame       = m_CurrentFrameId.load(std::memory_order_acquire);
    const bool crossFrame             = span.frameId != 0 && currentFrame != 0 && span.frameId != currentFrame;

    if (span.captureGeneration == m_CaptureGeneration.load(std::memory_order_acquire)) {
        RawEvent event;
        event.eventId           = span.eventId;
        event.parentEventId     = span.parentEventId;
        event.frameId           = span.frameId;
        event.flowId            = span.flowId;
        event.nameId            = span.nameId;
        event.threadId          = recorder.threadId;
        event.contextId         = span.domain == Domain::Gpu ? CurrentOpenGLContextId() : 0;
        event.captureGeneration = span.captureGeneration;
        event.startTicks        = span.startTicks;
        event.durationTicks     = durationTicks;
        event.cpuDurationTicks  = durationTicks;
        event.nameIndex         = span.nameIndex;
        event.parentNameIndex   = span.parentNameIndex;
        event.flags             = EventComplete | (mismatch ? EventMismatch : 0u) | (crossFrame ? EventCrossFrame : 0u);
        event.depth             = span.depth;
        event.domain            = span.domain;
        event.kind              = EventKind::Span;
        recorder.Append(event);
    }

    while (recorder.depth > 0 && !recorder.stack[recorder.depth - 1].active)
        --recorder.depth;

    if (sampleOverhead) {
        m_ProfilerOverheadNs.fetch_add(NowTicks() - overheadStart, std::memory_order_relaxed);
        m_ProfilerOverheadSamples.fetch_add(1, std::memory_order_relaxed);
    }
}

void PerformanceMonitor::RecordCounter(std::string_view key, double value, Domain domain, uint64_t flowId)
{
    if (key.empty() || GetCaptureMode() == CaptureMode::Off)
        return;
    Recorder &recorder = GetThreadRecorder();
    RawEvent event;
    event.eventId           = m_NextEventId.fetch_add(1, std::memory_order_relaxed);
    event.frameId           = m_CurrentFrameId.load(std::memory_order_acquire);
    event.flowId            = flowId;
    event.nameId            = HashName(key);
    event.threadId          = recorder.threadId;
    event.contextId         = domain == Domain::Gpu ? CurrentOpenGLContextId() : 0;
    event.captureGeneration = m_CaptureGeneration.load(std::memory_order_acquire);
    event.startTicks        = NowTicks();
    event.nameIndex         = recorder.InternName(key);
    event.flags             = EventComplete;
    event.valueCount        = 1;
    event.value             = value;
    event.domain            = domain;
    event.kind              = EventKind::Counter;
    recorder.Append(event);
}

void PerformanceMonitor::RecordValue(std::string_view key, uint64_t value0, uint64_t value1, uint64_t value2,
                                     Domain domain, uint64_t flowId)
{
    if (key.empty() || GetCaptureMode() == CaptureMode::Off)
        return;
    Recorder &recorder = GetThreadRecorder();
    RawEvent event;
    event.eventId           = m_NextEventId.fetch_add(1, std::memory_order_relaxed);
    event.frameId           = m_CurrentFrameId.load(std::memory_order_acquire);
    event.flowId            = flowId;
    event.nameId            = HashName(key);
    event.threadId          = recorder.threadId;
    event.contextId         = domain == Domain::Gpu ? CurrentOpenGLContextId() : 0;
    event.captureGeneration = m_CaptureGeneration.load(std::memory_order_acquire);
    event.startTicks        = NowTicks();
    event.nameIndex         = recorder.InternName(key);
    event.flags             = EventComplete;
    event.value0            = value0;
    event.value1            = value1;
    event.value2            = value2;
    event.valueCount        = 3;
    event.value             = static_cast<double>(value0);
    event.domain            = domain;
    event.kind              = EventKind::Counter;
    recorder.Append(event);
}

void PerformanceMonitor::RecordInstant(std::string_view key, Domain domain, uint64_t flowId)
{
    if (key.empty() || GetCaptureMode() == CaptureMode::Off)
        return;
    Recorder &recorder = GetThreadRecorder();
    RawEvent event;
    event.eventId           = m_NextEventId.fetch_add(1, std::memory_order_relaxed);
    event.frameId           = m_CurrentFrameId.load(std::memory_order_acquire);
    event.flowId            = flowId;
    event.nameId            = HashName(key);
    event.threadId          = recorder.threadId;
    event.contextId         = domain == Domain::Gpu ? CurrentOpenGLContextId() : 0;
    event.captureGeneration = m_CaptureGeneration.load(std::memory_order_acquire);
    event.startTicks        = NowTicks();
    event.nameIndex         = recorder.InternName(key);
    event.flags             = EventComplete;
    event.domain            = domain;
    event.kind              = EventKind::Instant;
    recorder.Append(event);
}

void PerformanceMonitor::RecordFlowBegin(std::string_view key, uint64_t flowId, Domain domain)
{
    if (key.empty() || flowId == 0 || GetCaptureMode() == CaptureMode::Off)
        return;
    Recorder &recorder = GetThreadRecorder();
    RawEvent event;
    event.eventId           = m_NextEventId.fetch_add(1, std::memory_order_relaxed);
    event.frameId           = m_CurrentFrameId.load(std::memory_order_acquire);
    event.flowId            = flowId;
    event.nameId            = HashName(key);
    event.threadId          = recorder.threadId;
    event.contextId         = domain == Domain::Gpu ? CurrentOpenGLContextId() : 0;
    event.captureGeneration = m_CaptureGeneration.load(std::memory_order_acquire);
    event.startTicks        = NowTicks();
    event.nameIndex         = recorder.InternName(key);
    event.flags             = EventComplete;
    event.domain            = domain;
    event.kind              = EventKind::FlowBegin;
    recorder.Append(event);
}

void PerformanceMonitor::RecordFlowStep(std::string_view key, uint64_t flowId, std::string_view step,
                                        Domain domain)
{
    if (key.empty() || step.empty() || flowId == 0 || GetCaptureMode() == CaptureMode::Off)
        return;
    Recorder &recorder = GetThreadRecorder();
    RawEvent event;
    event.eventId           = m_NextEventId.fetch_add(1, std::memory_order_relaxed);
    event.frameId           = m_CurrentFrameId.load(std::memory_order_acquire);
    event.flowId            = flowId;
    event.nameId            = HashName(key);
    event.stepNameId        = HashName(step);
    event.threadId          = recorder.threadId;
    event.contextId         = domain == Domain::Gpu ? CurrentOpenGLContextId() : 0;
    event.captureGeneration = m_CaptureGeneration.load(std::memory_order_acquire);
    event.startTicks        = NowTicks();
    event.nameIndex         = recorder.InternName(key);
    event.stepNameIndex     = recorder.InternName(step);
    event.flags             = EventComplete;
    event.domain            = domain;
    event.kind              = EventKind::FlowStep;
    recorder.Append(event);
}

void PerformanceMonitor::RecordFlowEnd(std::string_view key, uint64_t flowId, Domain domain)
{
    if (key.empty() || flowId == 0 || GetCaptureMode() == CaptureMode::Off)
        return;
    Recorder &recorder = GetThreadRecorder();
    RawEvent event;
    event.eventId           = m_NextEventId.fetch_add(1, std::memory_order_relaxed);
    event.frameId           = m_CurrentFrameId.load(std::memory_order_acquire);
    event.flowId            = flowId;
    event.nameId            = HashName(key);
    event.threadId          = recorder.threadId;
    event.contextId         = domain == Domain::Gpu ? CurrentOpenGLContextId() : 0;
    event.captureGeneration = m_CaptureGeneration.load(std::memory_order_acquire);
    event.startTicks        = NowTicks();
    event.nameIndex         = recorder.InternName(key);
    event.flags             = EventComplete;
    event.domain            = domain;
    event.kind              = EventKind::FlowEnd;
    recorder.Append(event);
}

void PerformanceMonitor::SetMetadata(std::string_view key, std::string_view value)
{
    if (key.empty())
        return;
    std::lock_guard lock(m_Storage->dataMutex);
    for (MetadataEntry &entry : m_Storage->metadata) {
        if (entry.key == key) {
            entry.value = value;
            return;
        }
    }
    m_Storage->metadata.push_back({std::string(key), std::string(value)});
}

PerformanceMonitor::GpuScope PerformanceMonitor::BeginGpuScope(std::string_view key, Domain domain, uint64_t flowId)
{
#if defined(TF3D_PROFILER_GPU) && !TF3D_PROFILER_GPU
    (void)key;
    (void)domain;
    (void)flowId;
    return {};
#else
    if (key.empty() || GetCaptureMode() != CaptureMode::Full || glfwGetCurrentContext() == nullptr)
        return {};

    Recorder &recorder       = GetThreadRecorder();
    const uint64_t contextId = reinterpret_cast<uint64_t>(glfwGetCurrentContext());
    GpuContext *context      = nullptr;
    for (GpuContext &candidate : recorder.gpuContexts) {
        if (candidate.contextId == contextId) {
            context = &candidate;
            break;
        }
    }
    if (context == nullptr) {
        recorder.gpuContexts.push_back(GpuContext{});
        context            = &recorder.gpuContexts.back();
        context->contextId = contextId;
    }

    if (!context->queriesCreated) {
        glGenQueries(static_cast<GLsizei>(kGpuQueryCapacity), context->beginQueries.data());
        glGenQueries(static_cast<GLsizei>(kGpuQueryCapacity), context->endQueries.data());
        context->queriesCreated = true;
        for (std::size_t index = 0; index < kGpuQueryCapacity; ++index) {
            context->slots[index].beginQuery = context->beginQueries[index];
            context->slots[index].endQuery   = context->endQueries[index];
        }
    }
    if (!context->clockCalibrated) {
        const uint64_t cpuBefore = NowTicks();
        GLint64 gpuNow           = 0;
        glGetInteger64v(GL_TIMESTAMP, &gpuNow);
        const uint64_t cpuAfter   = NowTicks();
        const uint64_t cpuMid     = cpuBefore + (cpuAfter - cpuBefore) / 2;
        context->gpuToCpuOffsetNs = static_cast<int64_t>(cpuMid) - gpuNow;
        context->clockCalibrated  = true;
    }

    GpuQuerySlot *slot    = nullptr;
    std::size_t slotIndex = 0;
    for (std::size_t offset = 0; offset < kGpuQueryCapacity; ++offset) {
        const std::size_t index = (context->nextSlot + offset) % kGpuQueryCapacity;
        if (!context->slots[index].active) {
            slot              = &context->slots[index];
            slotIndex         = index;
            context->nextSlot = (index + 1) % kGpuQueryCapacity;
            break;
        }
    }
    if (slot == nullptr) {
        m_DroppedGpuQueryCount.fetch_add(1, std::memory_order_relaxed);
        return {};
    }

    const uint64_t generation = m_CaptureGeneration.load(std::memory_order_acquire);
    slot->eventId             = m_NextEventId.fetch_add(1, std::memory_order_relaxed);
    slot->frameId             = m_CurrentFrameId.load(std::memory_order_acquire);
    slot->flowId              = flowId;
    slot->generation          = generation;
    slot->nameId              = HashName(key);
    slot->nameIndex           = recorder.InternName(key);
    slot->cpuStartTicks       = NowTicks();
    slot->cpuEndTicks         = 0;
    slot->domain              = domain;
    slot->flags               = EventComplete;
    slot->active              = true;
    slot->ended               = false;
    slot->invalid             = false;
    glQueryCounter(slot->beginQuery, GL_TIMESTAMP);
    m_PendingGpuQueryCount.fetch_add(1, std::memory_order_relaxed);
    return GpuScope(this, context, static_cast<uint32_t>(slotIndex), generation);
#endif
}

PerformanceMonitor::GpuScope PerformanceMonitor::BeginChildGpuScope(std::string_view key, Domain domain, uint64_t flowId)
{
    if (GetCaptureMode() != CaptureMode::Full)
        return {};
    return BeginGpuScope(ResolveChildKey(key), domain, flowId);
}

void PerformanceMonitor::GpuScope::End()
{
    if (m_Monitor == nullptr || m_State == nullptr)
        return;
    m_Monitor->EndGpuScope(m_State, m_Slot, m_Generation);
    m_Monitor    = nullptr;
    m_State      = nullptr;
    m_Slot       = 0;
    m_Generation = 0;
}

void PerformanceMonitor::EndGpuScope(void *statePointer, uint32_t slotIndex, uint64_t generation)
{
#if defined(TF3D_PROFILER_GPU) && !TF3D_PROFILER_GPU
    (void)statePointer;
    (void)slotIndex;
    (void)generation;
#else
    if (statePointer == nullptr || slotIndex >= kGpuQueryCapacity)
        return;
    GpuContext &context = *static_cast<GpuContext *>(statePointer);
    GpuQuerySlot &slot  = context.slots[slotIndex];
    if (!slot.active || slot.generation != generation || slot.ended)
        return;

    if (CurrentOpenGLContextId() == context.contextId) {
        glQueryCounter(slot.endQuery, GL_TIMESTAMP);
    } else {
        slot.invalid = true;
        slot.flags |= EventGpuUnavailable | EventMismatch;
    }
    slot.cpuEndTicks = NowTicks();
    slot.ended       = true;
    m_PendingGpuResultCount.fetch_add(1, std::memory_order_relaxed);
#endif
}

void PerformanceMonitor::PollGpuQueries(bool waitForResults)
{
#if defined(TF3D_PROFILER_GPU) && !TF3D_PROFILER_GPU
    (void)waitForResults;
    return;
#else
    if (glfwGetCurrentContext() == nullptr)
        return;
    Recorder &recorder            = GetThreadRecorder();
    const uint64_t currentContext = CurrentOpenGLContextId();
    for (GpuContext &context : recorder.gpuContexts) {
        if (context.contextId != currentContext)
            continue;
        for (GpuQuerySlot &slot : context.slots) {
            if (!slot.active || !slot.ended)
                continue;

            const auto retireSlot = [&](bool dropped) {
                slot.active  = false;
                slot.ended   = false;
                slot.invalid = false;
                m_PendingGpuResultCount.fetch_sub(1, std::memory_order_relaxed);
                m_PendingGpuQueryCount.fetch_sub(1, std::memory_order_relaxed);
                if (dropped)
                    m_DroppedGpuQueryCount.fetch_add(1, std::memory_order_relaxed);
            };

            if (slot.invalid || glIsQuery(slot.beginQuery) == GL_FALSE || glIsQuery(slot.endQuery) == GL_FALSE) {
                retireSlot(true);
                continue;
            }

            if (!waitForResults) {
                GLint available = GL_FALSE;
                glGetQueryObjectiv(slot.endQuery, GL_QUERY_RESULT_AVAILABLE, &available);
                if (available == GL_FALSE)
                    continue;
            }

            GLuint64 gpuStart = 0;
            GLuint64 gpuEnd   = 0;
            glGetQueryObjectui64v(slot.beginQuery, GL_QUERY_RESULT, &gpuStart);
            glGetQueryObjectui64v(slot.endQuery, GL_QUERY_RESULT, &gpuEnd);
            const bool valid                 = gpuEnd >= gpuStart;
            const uint64_t currentGeneration = m_CaptureGeneration.load(std::memory_order_acquire);
            if (valid && slot.generation == currentGeneration && IsCaptureGenerationActive(slot.generation)) {
                RawEvent event;
                event.eventId                 = slot.eventId;
                event.frameId                 = slot.frameId;
                event.flowId                  = slot.flowId;
                event.nameId                  = slot.nameId;
                event.threadId                = recorder.threadId;
                event.contextId               = context.contextId;
                event.captureGeneration       = slot.generation;
                const int64_t alignedGpuStart = static_cast<int64_t>(gpuStart) + context.gpuToCpuOffsetNs;
                event.startTicks              = alignedGpuStart >= 0 ? static_cast<uint64_t>(alignedGpuStart) : slot.cpuStartTicks;
                event.durationTicks           = gpuEnd - gpuStart;
                event.cpuDurationTicks        = slot.cpuEndTicks >= slot.cpuStartTicks ? slot.cpuEndTicks - slot.cpuStartTicks : 0;
                event.gpuDurationTicks        = gpuEnd - gpuStart;
                event.nameIndex               = slot.nameIndex;
                event.flags                   = slot.flags | EventGpuDelayed;
                if (slot.frameId != 0 && slot.frameId != m_CurrentFrameId.load(std::memory_order_acquire))
                    event.flags |= EventCrossFrame;
                event.domain = slot.domain;
                event.kind   = EventKind::Span;
                recorder.Append(event);
            }
            retireSlot(!valid);
        }
    }
#endif
}

void PerformanceMonitor::BeginFrame()
{
    PollGpuQueries();
    CollectCompletedEvents();
    const uint64_t frameId    = m_NextFrameId.fetch_add(1, std::memory_order_relaxed) + 1;
    const uint64_t startTicks = NowTicks();
    m_CurrentFrameId.store(frameId, std::memory_order_release);
    m_CurrentFrameStartTicks.store(startTicks, std::memory_order_release);
    if (IsCapturing())
        EnsureFrame(frameId, startTicks, false, 0.0);
}

void PerformanceMonitor::EndFrame()
{
    const uint64_t frameId    = m_CurrentFrameId.load(std::memory_order_acquire);
    const uint64_t startTicks = m_CurrentFrameStartTicks.load(std::memory_order_acquire);
    const uint64_t now        = NowTicks();
    if (IsCapturing()) {
        const double durationMs = now >= startTicks ? static_cast<double>(now - startTicks) / 1000000.0 : 0.0;
        EnsureFrame(frameId, startTicks, true, durationMs);
    }

    const bool drainGpu = m_GpuDrainRequested.exchange(false, std::memory_order_acq_rel);
    if (drainGpu &&
        glfwGetCurrentContext() != nullptr &&
        m_PendingGpuQueryCount.load(std::memory_order_acquire) != 0) {
        glFinish();
    }
    PollGpuQueries(drainGpu);
    CollectCompletedEvents();
}

void PerformanceMonitor::EnsureFrame(uint64_t frameId, uint64_t startTicks, bool complete, double durationMs)
{
    if (frameId == 0)
        return;
    std::lock_guard lock(m_Storage->dataMutex);
    const auto [iterator, inserted] = m_Storage->frameIndices.emplace(frameId, m_Storage->frames.size());
    if (inserted) {
        FrameSnapshot frame;
        frame.index      = frameId;
        frame.startTicks = startTicks;
        frame.durationMs = durationMs;
        frame.complete   = complete;
        m_Storage->frames.push_back(std::move(frame));
    } else {
        FrameSnapshot &frame = m_Storage->frames[iterator->second];
        if (frame.startTicks == 0)
            frame.startTicks = startTicks;
        if (complete) {
            frame.durationMs = durationMs;
            frame.complete   = true;
        }
    }
}

void PerformanceMonitor::AppendEvent(const EventSnapshot &event)
{
    EventSnapshot adjusted = event;
    std::lock_guard lock(m_Storage->dataMutex);
    if (adjusted.frameId == 0) {
        m_Storage->unframedEvents.push_back(std::move(adjusted));
        return;
    }

    const auto [iterator, inserted] = m_Storage->frameIndices.emplace(adjusted.frameId, m_Storage->frames.size());
    if (inserted) {
        FrameSnapshot frame;
        frame.index      = adjusted.frameId;
        frame.startTicks = adjusted.startTicks;
        m_Storage->frames.push_back(std::move(frame));
    }
    FrameSnapshot &frame = m_Storage->frames[iterator->second];
    if (frame.startTicks == 0)
        frame.startTicks = adjusted.startTicks;
    if (adjusted.startTicks >= frame.startTicks)
        adjusted.startOffsetMs = static_cast<double>(adjusted.startTicks - frame.startTicks) / 1000000.0;
    frame.events.push_back(std::move(adjusted));
}

void PerformanceMonitor::CollectCompletedEvents()
{
    std::vector<Recorder *> recorders;
    {
        std::lock_guard lock(m_Storage->registryMutex);
        recorders = m_Storage->recorders;
    }

    const uint64_t generation   = m_CaptureGeneration.load(std::memory_order_acquire);
    const auto nameFromRecorder = [](const Recorder &recorder, uint32_t index, uint64_t nameId) {
        if (index != kInvalidNameIndex && index < kNameCapacity &&
            recorder.names[index].ready.load(std::memory_order_acquire)) {
            return std::string(recorder.names[index].text.data());
        }
        return "#" + std::to_string(nameId);
    };
    for (Recorder *recorder : recorders) {
        const uint64_t published = recorder->publishedSequence.load(std::memory_order_acquire);
        uint64_t read            = recorder->readSequence.load(std::memory_order_relaxed);
        while (read < published) {
            const RawEvent raw = recorder->events[read % kEventCapacity];
            ++read;
            if (raw.captureGeneration != generation)
                continue;

            EventSnapshot event;
            event.eventId       = raw.eventId;
            event.parentEventId = raw.parentEventId;
            event.frameId       = raw.frameId;
            event.flowId        = raw.flowId;
            event.nameId        = raw.nameId;
            event.threadId      = raw.threadId;
            event.contextId     = raw.contextId;
            event.startTicks    = raw.startTicks;
            event.value0        = raw.value0;
            event.value1        = raw.value1;
            event.value2        = raw.value2;
            event.flags         = raw.flags;
            event.depth         = raw.depth;
            event.valueCount    = raw.valueCount;
            event.domain        = raw.domain;
            event.kind          = raw.kind;
            event.value         = raw.value;
            event.durationMs    = static_cast<double>(raw.durationTicks) / 1000000.0;
            event.cpuDurationMs = static_cast<double>(raw.cpuDurationTicks) / 1000000.0;
            event.gpuDurationMs = static_cast<double>(raw.gpuDurationTicks) / 1000000.0;
            event.key           = nameFromRecorder(*recorder, raw.nameIndex, raw.nameId);
            event.parentKey     = nameFromRecorder(*recorder, raw.parentNameIndex, 0);
            if (raw.stepNameId != 0)
                event.step = nameFromRecorder(*recorder, raw.stepNameIndex, raw.stepNameId);
            {
                std::lock_guard lock(m_Storage->registryMutex);
                event.threadName = recorder->threadName;
            }
            AppendEvent(event);
        }
        recorder->readSequence.store(read, std::memory_order_release);
    }
}

PerformanceMonitor::Snapshot PerformanceMonitor::CaptureSnapshot() const
{
    Snapshot snapshot;
    const CaptureMode currentMode    = GetCaptureMode();
    snapshot.isCapturing             = currentMode != CaptureMode::Off;
    snapshot.mode                    = snapshot.isCapturing
                                           ? currentMode
                                           : m_LastCaptureMode.load(std::memory_order_acquire);
    snapshot.droppedEventCount       = m_DroppedEventCount.load(std::memory_order_relaxed);
    snapshot.incompleteEventCount    = m_IncompleteEventCount.load(std::memory_order_relaxed);
    snapshot.pendingGpuQueryCount    = m_PendingGpuQueryCount.load(std::memory_order_relaxed);
    snapshot.pendingGpuResultCount   = m_PendingGpuResultCount.load(std::memory_order_relaxed);
    snapshot.openGpuScopeCount       = snapshot.pendingGpuQueryCount > snapshot.pendingGpuResultCount
                                           ? snapshot.pendingGpuQueryCount - snapshot.pendingGpuResultCount
                                           : 0;
    snapshot.droppedGpuQueryCount    = m_DroppedGpuQueryCount.load(std::memory_order_relaxed);
    snapshot.profilerOverheadNs      = m_ProfilerOverheadNs.load(std::memory_order_relaxed);
    snapshot.profilerOverheadSamples = m_ProfilerOverheadSamples.load(std::memory_order_relaxed);
    std::lock_guard lock(m_Storage->dataMutex);
    snapshot.capturedFrames = m_Storage->frames;
    snapshot.unframedEvents = m_Storage->unframedEvents;
    snapshot.metadata       = m_Storage->metadata;
    for (const auto &frame : snapshot.capturedFrames) {
        snapshot.capturedDurationMs += frame.durationMs;
        snapshot.capturedEventCount += frame.events.size();
    }
    snapshot.capturedEventCount += snapshot.unframedEvents.size();
    return snapshot;
}

void PerformanceMonitor::StartCapture(CaptureMode mode)
{
    if (mode == CaptureMode::Off)
        mode = CaptureMode::Cpu;
    m_CaptureGeneration.fetch_add(1, std::memory_order_acq_rel);
    m_GpuDrainRequested.store(false, std::memory_order_release);
    {
        std::lock_guard lock(m_Storage->dataMutex);
        m_Storage->frames.clear();
        m_Storage->frameIndices.clear();
        m_Storage->unframedEvents.clear();
    }
    m_DroppedEventCount.store(0, std::memory_order_relaxed);
    m_IncompleteEventCount.store(0, std::memory_order_relaxed);
    m_DroppedGpuQueryCount.store(0, std::memory_order_relaxed);
    m_ProfilerOverheadNs.store(0, std::memory_order_relaxed);
    m_ProfilerOverheadSamples.store(0, std::memory_order_relaxed);
    m_LastCaptureMode.store(mode, std::memory_order_release);
    m_CaptureMode.store(mode, std::memory_order_release);
    SetMetadata("capture/mode", CaptureModeName(mode));
    if (m_CurrentFrameId.load(std::memory_order_acquire) != 0)
        EnsureFrame(m_CurrentFrameId.load(std::memory_order_acquire), m_CurrentFrameStartTicks.load(std::memory_order_acquire), false, 0.0);
}

void PerformanceMonitor::StopCapture()
{
    if (!IsCapturing())
        return;
    CollectCompletedEvents();
    const bool hadGpuCapture = GetCaptureMode() == CaptureMode::Full;
    m_CaptureMode.store(CaptureMode::Off, std::memory_order_release);
    if (hadGpuCapture)
        m_GpuDrainRequested.store(true, std::memory_order_release);
    CollectCompletedEvents();
}

void PerformanceMonitor::ClearCapture()
{
    m_CaptureMode.store(CaptureMode::Off, std::memory_order_release);
    m_CaptureGeneration.fetch_add(1, std::memory_order_acq_rel);
    m_GpuDrainRequested.store(false, std::memory_order_release);
    {
        std::lock_guard lock(m_Storage->dataMutex);
        m_Storage->frames.clear();
        m_Storage->frameIndices.clear();
        m_Storage->unframedEvents.clear();
    }
    m_DroppedEventCount.store(0, std::memory_order_relaxed);
    m_IncompleteEventCount.store(0, std::memory_order_relaxed);
    m_DroppedGpuQueryCount.store(0, std::memory_order_relaxed);
    m_ProfilerOverheadNs.store(0, std::memory_order_relaxed);
    m_ProfilerOverheadSamples.store(0, std::memory_order_relaxed);
    m_LastCaptureMode.store(CaptureMode::Off, std::memory_order_release);
}

bool PerformanceMonitor::ExportChromeTrace(const std::string &path) const
{
    const Snapshot snapshot = CaptureSnapshot();
    std::ofstream output(path, std::ios::binary);
    if (!output)
        return false;

    struct TraceThread {
        uint64_t id = 0;
        std::string name;
    };

    std::vector<TraceThread> traceThreads;
    const auto addTraceThread = [&](const EventSnapshot &event) {
        const uint64_t id      = ChromeThreadId(event);
        const std::string name = ChromeThreadName(event);
        for (TraceThread &thread : traceThreads) {
            if (thread.id != id)
                continue;
            if (thread.name.rfind("Thread ", 0) == 0 && !name.empty())
                thread.name = name;
            return;
        }
        traceThreads.push_back({id, name});
    };

    uint64_t firstTicks = std::numeric_limits<uint64_t>::max();
    auto findFirst      = [&](const EventSnapshot &event) {
        firstTicks = std::min(firstTicks, event.startTicks);
        addTraceThread(event);
    };
    for (const auto &frame : snapshot.capturedFrames)
        for (const auto &event : frame.events)
            findFirst(event);
    for (const auto &event : snapshot.unframedEvents)
        findFirst(event);
    if (firstTicks == std::numeric_limits<uint64_t>::max())
        firstTicks = 0;

    bool first = true;
    output << "{\"traceEvents\":[";

    const auto writeComma = [&]() {
        if (!first)
            output << ',';
        first = false;
    };
    const auto writeMetadataString = [&](std::string_view name, uint64_t threadId, std::string_view key,
                                         std::string_view value) {
        writeComma();
        output << "{\"name\":\"" << JsonEscape(name)
               << "\",\"ph\":\"M\",\"pid\":1,\"tid\":" << threadId
               << ",\"ts\":0,\"args\":{\"" << JsonEscape(key) << "\":\""
               << JsonEscape(value) << "\"}}";
    };
    const auto writeMetadataNumber = [&](std::string_view name, uint64_t threadId, std::string_view key,
                                         uint64_t value) {
        writeComma();
        output << "{\"name\":\"" << JsonEscape(name)
               << "\",\"ph\":\"M\",\"pid\":1,\"tid\":" << threadId
               << ",\"ts\":0,\"args\":{\"" << JsonEscape(key) << "\":" << value << "}}";
    };

    auto metadataValue = [&](std::string_view key, std::string_view fallback) {
        for (const MetadataEntry &entry : snapshot.metadata) {
            if (entry.key == key)
                return entry.value;
        }
        return std::string(fallback);
    };

    writeMetadataString("process_name", 0, "name", metadataValue("process/name", "TerraForge3D"));
    writeMetadataNumber("process_sort_index", 0, "sort_index", 0);
    for (std::size_t index = 0; index < traceThreads.size(); ++index) {
        const TraceThread &thread = traceThreads[index];
        writeMetadataString("thread_name", thread.id, "name", thread.name);
        writeMetadataNumber("thread_sort_index", thread.id, "sort_index", static_cast<uint64_t>(index));
    }

    auto writeEvent = [&](const EventSnapshot &event) {
        writeComma();
        const uint64_t traceThreadId = ChromeThreadId(event);
        const double timestampUs     = event.startTicks >= firstTicks
                                           ? static_cast<double>(event.startTicks - firstTicks) / 1000.0
                                           : 0.0;
        output << "{\"name\":\"" << JsonEscape(event.key) << "\",\"cat\":\""
               << JsonEscape(DomainName(event.domain)) << "\",\"pid\":1,\"tid\":" << traceThreadId;
        if (event.kind == EventKind::Span) {
            output << ",\"ph\":\"X\",\"ts\":" << std::fixed << std::setprecision(3) << timestampUs
                   << ",\"dur\":" << event.durationMs * 1000.0;
        } else if (event.kind == EventKind::Counter) {
            output << ",\"ph\":\"C\",\"ts\":" << std::fixed << std::setprecision(3) << timestampUs;
        } else if (event.kind == EventKind::Instant) {
            output << ",\"ph\":\"i\",\"s\":\"t\",\"ts\":" << std::fixed << std::setprecision(3) << timestampUs;
        } else if (event.kind == EventKind::FlowBegin) {
            output << ",\"ph\":\"s\",\"id\":\"" << event.flowId << "\",\"ts\":" << std::fixed << std::setprecision(3) << timestampUs;
        } else if (event.kind == EventKind::FlowStep) {
            output << ",\"ph\":\"t\",\"id\":\"" << event.flowId << "\",\"ts\":" << std::fixed << std::setprecision(3) << timestampUs;
        } else {
            output << ",\"ph\":\"f\",\"id\":\"" << event.flowId << "\",\"ts\":" << std::fixed << std::setprecision(3) << timestampUs;
        }

        bool firstArgument            = true;
        const auto writeArgumentComma = [&]() {
            if (!firstArgument)
                output << ',';
            firstArgument = false;
        };
        const auto writeArgumentString = [&](std::string_view key, std::string_view value) {
            writeArgumentComma();
            output << "\"" << JsonEscape(key) << "\":\"" << JsonEscape(value) << "\"";
        };
        const auto writeArgumentInteger = [&](std::string_view key, uint64_t value) {
            writeArgumentComma();
            output << "\"" << JsonEscape(key) << "\":" << value;
        };
        const auto writeArgumentDouble = [&](std::string_view key, double value) {
            writeArgumentComma();
            output << "\"" << JsonEscape(key) << "\":";
            if (std::isfinite(value))
                output << std::fixed << std::setprecision(3) << value;
            else
                output << 0.0;
        };

        output << ",\"args\":{";
        writeArgumentInteger("event_id", event.eventId);
        writeArgumentInteger("frame_id", event.frameId);
        writeArgumentInteger("flow_id", event.flowId);
        writeArgumentString("domain", DomainName(event.domain));
        writeArgumentInteger("flags", event.flags);
        if (event.frameId != 0)
            writeArgumentDouble("frame_offset_us", event.startOffsetMs * 1000.0);
        const std::string flagNames = EventFlagNames(event.flags);
        if (!flagNames.empty())
            writeArgumentString("flag_names", flagNames);
        if (event.parentEventId != 0)
            writeArgumentInteger("parent_event_id", event.parentEventId);
        if (event.parentEventId != 0 && !event.parentKey.empty())
            writeArgumentString("parent_key", event.parentKey);
        if (event.kind == EventKind::Span) {
            writeArgumentInteger("depth", event.depth);
            writeArgumentDouble("duration_us", event.durationMs * 1000.0);
            writeArgumentDouble("cpu_duration_us", event.cpuDurationMs * 1000.0);
            writeArgumentDouble("gpu_duration_us", event.gpuDurationMs * 1000.0);
        }
        if (event.valueCount != 0)
            writeArgumentInteger("value_count", event.valueCount);
        if (event.kind == EventKind::Counter) {
            if (event.valueCount == 1) {
                writeArgumentDouble("value", event.value);
            } else if (event.valueCount >= 3) {
                writeArgumentInteger("value0", event.value0);
                writeArgumentInteger("value1", event.value1);
                writeArgumentInteger("value2", event.value2);
            }
        }
        if (event.kind == EventKind::FlowStep && !event.step.empty())
            writeArgumentString("step", event.step);
        if (event.contextId != 0)
            writeArgumentInteger("context_id", event.contextId);
        if (traceThreadId != event.threadId) {
            writeArgumentInteger("source_thread_id", event.threadId);
            if (!event.threadName.empty())
                writeArgumentString("source_thread_name", event.threadName);
        }
        if (IsGpuEvent(event))
            writeArgumentString("track", "gpu");
        output << "}}";
    };
    for (const auto &frame : snapshot.capturedFrames)
        for (const auto &event : frame.events)
            writeEvent(event);
    for (const auto &event : snapshot.unframedEvents)
        writeEvent(event);
    output << "],\"metadata\":{";
    bool firstMetadata            = true;
    const auto writeMetadataComma = [&]() {
        if (!firstMetadata)
            output << ',';
        firstMetadata = false;
    };
    const auto writeMetadataStringValue = [&](std::string_view key, std::string_view value) {
        writeMetadataComma();
        output << "\"" << JsonEscape(key) << "\":\"" << JsonEscape(value) << "\"";
    };
    const auto writeMetadataIntegerValue = [&](std::string_view key, uint64_t value) {
        writeMetadataComma();
        output << "\"" << JsonEscape(key) << "\":" << value;
    };
    const auto writeMetadataDoubleValue = [&](std::string_view key, double value) {
        writeMetadataComma();
        output << "\"" << JsonEscape(key) << "\":" << std::fixed << std::setprecision(3) << value;
    };

    writeMetadataStringValue("format", "chrome-trace-event");
    writeMetadataStringValue("profilerMode", CaptureModeName(snapshot.mode));
    writeMetadataStringValue("processName", metadataValue("process/name", "TerraForge3D"));
    writeMetadataIntegerValue("capturedEventCount", snapshot.capturedEventCount);
    writeMetadataDoubleValue("capturedDurationMs", snapshot.capturedDurationMs);
    writeMetadataIntegerValue("droppedEvents", snapshot.droppedEventCount);
    writeMetadataIntegerValue("incompleteEvents", snapshot.incompleteEventCount);
    writeMetadataIntegerValue("pendingGpuQueries", snapshot.pendingGpuQueryCount);
    writeMetadataIntegerValue("openGpuScopes", snapshot.openGpuScopeCount);
    writeMetadataIntegerValue("pendingGpuResults", snapshot.pendingGpuResultCount);
    writeMetadataIntegerValue("droppedGpuQueries", snapshot.droppedGpuQueryCount);
    writeMetadataIntegerValue("profilerOverheadNs", snapshot.profilerOverheadNs);
    writeMetadataIntegerValue("profilerOverheadSamples", snapshot.profilerOverheadSamples);
    writeMetadataComma();
    output << "\"environment\":{";
    for (std::size_t index = 0; index < snapshot.metadata.size(); ++index) {
        if (index != 0)
            output << ',';
        output << "\"" << JsonEscape(snapshot.metadata[index].key) << "\":\""
               << JsonEscape(snapshot.metadata[index].value) << "\"";
    }
    output << "}}}";
    return static_cast<bool>(output);
}

void PerformanceMonitor::RenderUI(bool *windowOpen)
{
    if (windowOpen == nullptr || !*windowOpen)
        return;

    TF3D_PROFILE_SCOPE_DOMAIN("profiler/ui", Domain::Ui);
    CollectCompletedEvents();
    Snapshot snapshot = CaptureSnapshot();
    if (!ImGui::Begin("Performance Monitor", windowOpen)) {
        ImGui::End();
        return;
    }

    static std::string exportStatus;
    bool refreshSnapshot = false;
    if (snapshot.isCapturing) {
        if (ImGui::Button("Stop")) {
            StopCapture();
            refreshSnapshot = true;
        }
    } else if (ImGui::Button("Start")) {
        StartCapture();
        refreshSnapshot = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Export JSON")) {
        std::string path = ShowSaveFileDialog("*.json");
        if (!path.empty()) {
            if (path.size() < 5 || path.compare(path.size() - 5, 5, ".json") != 0)
                path += ".json";
            exportStatus = ExportChromeTrace(path) ? "Saved: " + path : "Failed: " + path;
        }
    }
    if (refreshSnapshot)
        snapshot = CaptureSnapshot();
    if (!exportStatus.empty()) {
        ImGui::TextDisabled("%s", exportStatus.c_str());
    }

    std::size_t completeFrameCount = 0;
    double totalFrameMs            = 0.0;
    double maximumFrameMs          = 0.0;
    for (const auto &frame : snapshot.capturedFrames) {
        if (!frame.complete)
            continue;
        ++completeFrameCount;
        totalFrameMs += frame.durationMs;
        maximumFrameMs = std::max(maximumFrameMs, frame.durationMs);
    }

    ImGui::Separator();
    ImGui::TextDisabled("Capture");
    if (snapshot.isCapturing)
        ImGui::Text("Recording (%s)", CaptureModeName(snapshot.mode));
    else if (snapshot.capturedEventCount > 0)
        ImGui::Text("Stopped");
    else
        ImGui::Text("Idle");

    ImGui::TextDisabled("Frames");
    ImGui::Text("%zu complete | %zu events | %.3f ms captured",
                completeFrameCount, snapshot.capturedEventCount, snapshot.capturedDurationMs);
    if (completeFrameCount > 0) {
        ImGui::Text("%.3f ms average | %.3f ms max",
                    totalFrameMs / static_cast<double>(completeFrameCount), maximumFrameMs);
    }

    ImGui::TextDisabled("Health");
    ImGui::Text("%llu dropped | %llu incomplete | %llu GPU pending | %llu GPU dropped",
                static_cast<unsigned long long>(snapshot.droppedEventCount),
                static_cast<unsigned long long>(snapshot.incompleteEventCount),
                static_cast<unsigned long long>(snapshot.pendingGpuQueryCount),
                static_cast<unsigned long long>(snapshot.droppedGpuQueryCount));
    if (snapshot.pendingGpuQueryCount > 0)
        ImGui::TextDisabled("GPU pending detail: %llu open scopes | %llu awaiting results",
                            static_cast<unsigned long long>(snapshot.openGpuScopeCount),
                            static_cast<unsigned long long>(snapshot.pendingGpuResultCount));
    if (snapshot.profilerOverheadSamples > 0)
        ImGui::TextDisabled("Profiler overhead: %.3f ms across %llu samples",
                            static_cast<double>(snapshot.profilerOverheadNs) / 1000000.0,
                            static_cast<unsigned long long>(snapshot.profilerOverheadSamples));
    ImGui::End();
}
