#include "Base/AsyncTextureReadback.h"

#include <algorithm>
#include <cstring>

namespace tf3d::base
{
    AsyncTextureReadback::AsyncTextureReadback(std::size_t slotCount)
        : m_Slots(std::max<std::size_t>(slotCount, 1))
    {
    }

    AsyncTextureReadback::~AsyncTextureReadback()
    {
        Release();
    }

    bool AsyncTextureReadback::EnsureBuffers(std::size_t byteSize)
    {
        if (byteSize == 0)
            return false;
        if (m_ByteSize == byteSize && !m_Slots.empty() && m_Slots.front().pixelPackBuffer != 0)
            return true;
        if (m_PendingCount != 0)
            return false;

        GLint previousPixelPackBuffer = 0;
        glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPixelPackBuffer);
        ReleaseBuffers();
        m_ByteSize = byteSize;
        for (auto &slot : m_Slots) {
            glGenBuffers(1, &slot.pixelPackBuffer);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.pixelPackBuffer);
            glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(m_ByteSize), nullptr, GL_STREAM_READ);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(previousPixelPackBuffer));
        return true;
    }

    void AsyncTextureReadback::ReleaseBuffers()
    {
        GLint previousPixelPackBuffer = 0;
        glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPixelPackBuffer);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        for (auto &slot : m_Slots) {
            if (slot.fence != nullptr) {
                glDeleteSync(slot.fence);
                slot.fence = nullptr;
            }
            if (slot.pixelPackBuffer != 0) {
                glDeleteBuffers(1, &slot.pixelPackBuffer);
                slot.pixelPackBuffer = 0;
            }
            slot.token   = 0;
            slot.pending = false;
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(previousPixelPackBuffer));
        m_PendingCount = 0;
        m_NextSlot     = 0;
    }

    bool AsyncTextureReadback::QueueTexture2D(uint32_t textureRendererID, uint32_t format, uint32_t type,
                                              std::size_t byteSize, uint64_t token)
    {
        if (textureRendererID == 0 || !EnsureBuffers(byteSize) || m_Slots.empty())
            return false;

        std::size_t slotIndex = m_NextSlot;
        for (std::size_t offset = 0; offset < m_Slots.size(); ++offset) {
            const std::size_t candidate = (m_NextSlot + offset) % m_Slots.size();
            if (!m_Slots[candidate].pending) {
                slotIndex = candidate;
                break;
            }
        }

        auto &slot = m_Slots[slotIndex];
        if (slot.pending || slot.pixelPackBuffer == 0)
            return false;

        GLint previousActiveTexture = GL_TEXTURE0;
        GLint previousTexture       = 0;
        GLint previousPixelPack     = 0;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
        glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPixelPack);

        glBindTexture(GL_TEXTURE_2D, textureRendererID);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.pixelPackBuffer);
        glGetTexImage(GL_TEXTURE_2D, 0, format, type, nullptr);
        slot.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(previousPixelPack));
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        glActiveTexture(previousActiveTexture);

        if (slot.fence == nullptr) {
            glFinish();
            return false;
        }

        slot.token   = token;
        slot.pending = true;
        ++m_PendingCount;
        m_NextSlot = (slotIndex + 1) % m_Slots.size();
        return true;
    }

    std::optional<AsyncTextureReadback::Result> AsyncTextureReadback::Poll()
    {
        for (auto &slot : m_Slots) {
            if (!slot.pending || slot.fence == nullptr)
                continue;

            const GLenum waitResult = glClientWaitSync(slot.fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
            if (waitResult == GL_TIMEOUT_EXPIRED)
                continue;

            const bool ready = waitResult == GL_ALREADY_SIGNALED || waitResult == GL_CONDITION_SATISFIED;
            std::optional<Result> result;
            if (ready) {
                GLint previousPixelPack = 0;
                glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPixelPack);
                glBindBuffer(GL_PIXEL_PACK_BUFFER, slot.pixelPackBuffer);
                void *mapped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, static_cast<GLsizeiptr>(m_ByteSize), GL_MAP_READ_BIT);
                if (mapped != nullptr) {
                    Result completed;
                    completed.token = slot.token;
                    completed.data.resize(m_ByteSize);
                    std::memcpy(completed.data.data(), mapped, m_ByteSize);
                    if (glUnmapBuffer(GL_PIXEL_PACK_BUFFER) == GL_TRUE)
                        result = std::move(completed);
                }
                glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(previousPixelPack));
            }

            glDeleteSync(slot.fence);
            slot.fence   = nullptr;
            slot.token   = 0;
            slot.pending = false;
            --m_PendingCount;
            return result;
        }
        return std::nullopt;
    }

    void AsyncTextureReadback::Release()
    {
        ReleaseBuffers();
        m_ByteSize = 0;
    }
} // namespace tf3d::base
