#pragma once

#include <atomic>
#include <concepts>
#include <cstdint>
#include <functional>
#include <mutex>
#include <utility>

namespace tf3d::base
{
    class RevisionTracker
    {
    public:
        using Revision = uint64_t;

        explicit RevisionTracker(Revision publishedRevision = 0,
                                 Revision processedRevision = 0) noexcept
            : m_PublishedRevision(publishedRevision), m_ProcessedRevision(processedRevision)
        {
        }

        inline Revision Publish() noexcept
        {
            return m_PublishedRevision.fetch_add(1, std::memory_order_acq_rel) + 1;
        }

        inline void MarkProcessed(Revision revision) noexcept
        {
            m_ProcessedRevision.store(revision, std::memory_order_release);
        }

        inline bool RequiresUpdate() const noexcept
        {
            return PublishedRevision() != ProcessedRevision();
        }

        inline Revision PublishedRevision() const noexcept
        {
            return m_PublishedRevision.load(std::memory_order_acquire);
        }

        inline Revision ProcessedRevision() const noexcept
        {
            return m_ProcessedRevision.load(std::memory_order_acquire);
        }

    private:
        std::atomic<Revision> m_PublishedRevision;
        std::atomic<Revision> m_ProcessedRevision;
    };

    template <typename T>
    class GeneratorState
    {
    public:
        using Value    = T;
        using Revision = RevisionTracker::Revision;

        struct Snapshot {
            using Revision = RevisionTracker::Revision;

            T value;
            Revision revision = 0;
        };

        GeneratorState()
            requires std::default_initializable<T>
            : m_Value{},
              m_Tracker(1)
        {
        }

        explicit GeneratorState(T value)
            : m_Value(std::move(value)),
              m_Tracker(1)
        {
        }

        GeneratorState(const GeneratorState &)            = delete;
        GeneratorState &operator=(const GeneratorState &) = delete;

        Snapshot Capture() const
        {
            std::lock_guard lock(m_Mutex);
            return Snapshot{m_Value, m_Tracker.PublishedRevision()};
        }

        template <typename Function>
        bool Edit(Function &&function)
        {
            std::lock_guard lock(m_Mutex);
            if (!std::invoke(std::forward<Function>(function), m_Value)) {
                return false;
            }

            m_Tracker.Publish();
            return true;
        }

        void Replace(T value)
        {
            std::lock_guard lock(m_Mutex);
            m_Value = std::move(value);
            m_Tracker.Publish();
        }

        Revision Publish()
        {
            std::lock_guard lock(m_Mutex);
            return m_Tracker.Publish();
        }

        void MarkProcessed(Revision revision)
        {
            std::lock_guard lock(m_Mutex);
            m_Tracker.MarkProcessed(revision);
        }

        bool RequiresUpdate() const noexcept
        {
            return m_Tracker.RequiresUpdate();
        }

        Revision PublishedRevision() const noexcept
        {
            return m_Tracker.PublishedRevision();
        }

        Revision ProcessedRevision() const noexcept
        {
            return m_Tracker.ProcessedRevision();
        }

    private:
        mutable std::mutex m_Mutex;
        T m_Value;
        RevisionTracker m_Tracker;
    };

} // namespace tf3d::base
