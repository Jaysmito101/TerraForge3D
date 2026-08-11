#pragma once

#include <atomic>
#include <cstdint>

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

} // namespace tf3d::base
