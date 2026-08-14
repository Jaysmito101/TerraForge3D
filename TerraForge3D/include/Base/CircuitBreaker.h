#pragma once

#include <chrono>
#include <cstddef>
#include <mutex>

namespace tf3d::base
{

    class CircuitBreaker
    {
    public:
        enum class State {
            Closed,
            Open,
            HalfOpen
        };

        using Clock = std::chrono::steady_clock;

        explicit CircuitBreaker(std::size_t failureThreshold           = 5,
                                std::chrono::milliseconds openDuration = std::chrono::seconds(30))
            : m_FailureThreshold(failureThreshold == 0 ? 1 : failureThreshold),
              m_OpenDuration(openDuration.count() > 0 ? openDuration : std::chrono::milliseconds(1))
        {
        }

        CircuitBreaker(const CircuitBreaker &)            = delete;
        CircuitBreaker &operator=(const CircuitBreaker &) = delete;

        bool AllowRequest()
        {
            std::lock_guard lock(m_Mutex);
            if (m_State == State::Closed) {
                return true;
            }
            if (m_State == State::HalfOpen) {
                return false;
            }
            if (Clock::now() < m_OpenUntil) {
                return false;
            }

            m_State = State::HalfOpen;
            return true;
        }

        void RecordSuccess()
        {
            std::lock_guard lock(m_Mutex);
            if (m_State == State::Open) {
                return;
            }

            m_State        = State::Closed;
            m_FailureCount = 0;
            m_OpenUntil    = Clock::time_point::min();
        }

        void RecordFailure()
        {
            std::lock_guard lock(m_Mutex);
            if (m_State == State::Open) {
                return;
            }

            if (m_State == State::HalfOpen) {
                OpenLocked(Clock::now());
                return;
            }

            if (++m_FailureCount >= m_FailureThreshold) {
                OpenLocked(Clock::now());
            }
        }

        void AbortRequest()
        {
            std::lock_guard lock(m_Mutex);
            if (m_State == State::HalfOpen) {
                OpenLocked(Clock::now());
            }
        }

        void Reset()
        {
            std::lock_guard lock(m_Mutex);
            m_State        = State::Closed;
            m_FailureCount = 0;
            m_OpenUntil    = Clock::time_point::min();
        }

        State GetState() const
        {
            std::lock_guard lock(m_Mutex);
            return m_State;
        }

        bool IsOpen() const
        {
            std::lock_guard lock(m_Mutex);
            return m_State == State::Open && Clock::now() < m_OpenUntil;
        }

        std::size_t FailureCount() const
        {
            std::lock_guard lock(m_Mutex);
            return m_FailureCount;
        }

    private:
        void OpenLocked(Clock::time_point now)
        {
            m_State     = State::Open;
            m_OpenUntil = now + m_OpenDuration;
        }

        mutable std::mutex m_Mutex;
        const std::size_t m_FailureThreshold;
        const std::chrono::milliseconds m_OpenDuration;
        State m_State                 = State::Closed;
        std::size_t m_FailureCount    = 0;
        Clock::time_point m_OpenUntil = Clock::time_point::min();
    };

} // namespace tf3d::base
