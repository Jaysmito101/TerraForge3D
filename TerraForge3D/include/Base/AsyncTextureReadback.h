#pragma once

#include <cstddef>
#include <cstdint>
#include <glad/gl.h>
#include <optional>
#include <utility>
#include <vector>

namespace tf3d::base
{
    class AsyncTextureReadback
    {
    public:
        struct Result {
            uint64_t token = 0;
            std::vector<std::byte> data;
        };

        explicit AsyncTextureReadback(std::size_t slotCount = 3);
        ~AsyncTextureReadback();

        AsyncTextureReadback(const AsyncTextureReadback &)            = delete;
        AsyncTextureReadback &operator=(const AsyncTextureReadback &) = delete;

        bool QueueTexture2D(uint32_t textureRendererID, uint32_t format, uint32_t type,
                            std::size_t byteSize, uint64_t token);
        std::optional<Result> Poll();
        void Release();

        inline bool HasAvailableSlot() const
        {
            return m_PendingCount < m_Slots.size();
        }

    private:
        struct Slot {
            GLuint pixelPackBuffer = 0;
            GLsync fence           = nullptr;
            uint64_t token         = 0;
            bool pending           = false;
        };

        bool EnsureBuffers(std::size_t byteSize);
        void ReleaseBuffers();

        std::vector<Slot> m_Slots;
        std::size_t m_ByteSize     = 0;
        std::size_t m_NextSlot     = 0;
        std::size_t m_PendingCount = 0;
    };
} // namespace tf3d::base

using tf3d::base::AsyncTextureReadback;
