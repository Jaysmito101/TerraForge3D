#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>

namespace tf3d::generators
{

    enum class GenerationDirtyScope : uint32_t {
        None = 0,
        Biomes = 1u << 0,
        Mixer = 1u << 1,
        Structure = 1u << 2,
        Force = 1u << 3,
        AllBiomes = 1u << 4
    };

    struct GenerationDirtyState {
        uint32_t mask = 0;

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

        inline void Mark(GenerationDirtyScope scope)
        {
            m_PendingMask.fetch_or(static_cast<uint32_t>(scope), std::memory_order_release);
        }

        inline void MarkBiomes()
        {
            Mark(GenerationDirtyScope::Biomes);
        }

        inline void MarkAllBiomes()
        {
            Mark(GenerationDirtyScope::AllBiomes);
        }

        inline void MarkMixer()
        {
            Mark(GenerationDirtyScope::Mixer);
        }

        inline void MarkStructure()
        {
            Mark(GenerationDirtyScope::Structure);
        }

        inline void MarkForce()
        {
            Mark(GenerationDirtyScope::Force);
        }

        inline bool IsDirty() const
        {
            return m_PendingMask.load(std::memory_order_acquire) != 0;
        }

        inline GenerationDirtyState Consume()
        {
            return {m_PendingMask.exchange(0, std::memory_order_acq_rel)};
        }

        inline StateLock AcquireStateLock()
        {
            return StateLock(m_StateMutex);
        }

    private:
        std::atomic<uint32_t> m_PendingMask = 0;
        std::mutex m_StateMutex;
    };

} // namespace tf3d::generators
