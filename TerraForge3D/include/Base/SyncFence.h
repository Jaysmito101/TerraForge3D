#pragma once

#include <glad/gl.h>

#include <utility>

namespace tf3d::base
{
    class SyncFence
    {
    public:
        explicit SyncFence() noexcept : m_Handle(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0)) {}
        explicit SyncFence(GLsync handle) noexcept : m_Handle(handle) {}

        ~SyncFence()
        {
            std::move(*this).Consume();
        }

        SyncFence(const SyncFence &)            = delete;
        SyncFence &operator=(const SyncFence &) = delete;

        SyncFence(SyncFence &&other) noexcept : m_Handle(std::exchange(other.m_Handle, nullptr))
        {
        }

        SyncFence &operator=(SyncFence &&other) noexcept
        {
            if (this != &other) {
                std::move(*this).Consume();
                m_Handle = std::exchange(other.m_Handle, nullptr);
            }
            return *this;
        }

        explicit operator bool() const noexcept
        {
            return m_Handle != nullptr;
        }

        GLenum ClientWait(GLbitfield flags, GLuint64 timeout) const noexcept
        {
            return m_Handle != nullptr ? glClientWaitSync(m_Handle, flags, timeout) : GL_WAIT_FAILED;
        }

        void Consume() && noexcept
        {
            if (m_Handle != nullptr) {
                glDeleteSync(m_Handle);
                m_Handle = nullptr;
            }
        }

    private:
        GLsync m_Handle = nullptr;
    };
} // namespace tf3d::base
