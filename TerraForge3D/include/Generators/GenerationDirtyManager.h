#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>

namespace tf3d::generators
{

    enum class GenerationDirtyScope : uint32_t {
        None      = 0,
        Biomes    = 1u << 0,
        Mixer     = 1u << 1,
        Structure = 1u << 2,
        Force     = 1u << 3,
        AllBiomes = 1u << 4
    };

    enum class GenerationDirtyCause : uint32_t {
        Unknown  = 0,
        UiEdit   = 1u << 0,
        FileLoad = 1u << 1,
        Resize   = 1u << 2,
        External = 1u << 3,
        Force    = 1u << 4,
    };

    struct GenerationDirtyState {
        uint32_t mask      = 0;
        uint32_t causeMask = 0;
        uint64_t revision  = 0;

        inline bool IsDirty() const
        {
            return mask != 0;
        }

        inline bool Has(GenerationDirtyScope scope) const
        {
            return (mask & static_cast<uint32_t>(scope)) != 0;
        }

        inline bool RequiresForce() const
        {
            return Has(GenerationDirtyScope::Force);
        }
    };

    class GenerationDirtyManager
    {
    public:
        using StateLock = std::unique_lock<std::mutex>;

        inline void Mark(GenerationDirtyScope scope, GenerationDirtyCause cause = GenerationDirtyCause::Unknown)
        {
            std::lock_guard lock(m_DirtyMutex);
            m_PendingMask.fetch_or(static_cast<uint32_t>(scope), std::memory_order_release);
            m_CauseMask.fetch_or(static_cast<uint32_t>(cause), std::memory_order_release);
            m_Revision.fetch_add(1, std::memory_order_acq_rel);
        }

        inline void MarkBiomes(GenerationDirtyCause cause = GenerationDirtyCause::UiEdit)
        {
            Mark(GenerationDirtyScope::Biomes, cause);
        }

        inline void MarkAllBiomes(GenerationDirtyCause cause = GenerationDirtyCause::UiEdit)
        {
            Mark(GenerationDirtyScope::AllBiomes, cause);
        }

        inline void MarkMixer(GenerationDirtyCause cause = GenerationDirtyCause::UiEdit)
        {
            Mark(GenerationDirtyScope::Mixer, cause);
        }

        inline void MarkStructure(GenerationDirtyCause cause = GenerationDirtyCause::UiEdit)
        {
            Mark(GenerationDirtyScope::Structure, cause);
        }

        inline void MarkForce(GenerationDirtyCause cause = GenerationDirtyCause::Force)
        {
            Mark(GenerationDirtyScope::Force, cause);
        }

        inline bool IsDirty() const
        {
            return m_PendingMask.load(std::memory_order_acquire) != 0;
        }

        inline GenerationDirtyState Snapshot() const
        {
            std::lock_guard lock(m_DirtyMutex);
            return {m_PendingMask.load(std::memory_order_acquire),
                    m_CauseMask.load(std::memory_order_acquire),
                    m_Revision.load(std::memory_order_acquire)};
        }

        inline GenerationDirtyState Consume()
        {
            std::lock_guard lock(m_DirtyMutex);
            return {m_PendingMask.exchange(0, std::memory_order_acq_rel),
                    m_CauseMask.exchange(0, std::memory_order_acq_rel),
                    m_Revision.load(std::memory_order_acquire)};
        }

        inline bool ConsumeIfRevision(uint64_t expectedRevision)
        {
            std::lock_guard lock(m_DirtyMutex);
            if (m_Revision.load(std::memory_order_acquire) != expectedRevision) {
                return false;
            }

            m_PendingMask.store(0, std::memory_order_release);
            m_CauseMask.store(0, std::memory_order_release);
            return true;
        }

        inline StateLock AcquireStateLock()
        {
            return StateLock(m_StateMutex);
        }

    private:
        std::atomic<uint32_t> m_PendingMask = 0;
        std::atomic<uint32_t> m_CauseMask   = 0;
        std::atomic<uint64_t> m_Revision    = 0;
        mutable std::mutex m_DirtyMutex;
        std::mutex m_StateMutex;
    };

} // namespace tf3d::generators
