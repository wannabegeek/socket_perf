//
// Created by tom on 10/10/2021.
//

#ifndef TFSPINLOCK_H
#define TFSPINLOCK_H

#include <iostream>
#include <atomic>

namespace tf {

    class spinlock {
        std::atomic_flag m_locked = ATOMIC_FLAG_INIT;

    public:
        inline void lock() noexcept {
            while (m_locked.test_and_set(std::memory_order_acquire));
        }

        inline void unlock() noexcept {
            m_locked.clear(std::memory_order_release);
        }
    };

    class spinlock_auto {
        spinlock &m_lock;
    public:
        explicit spinlock_auto(spinlock &lock) noexcept : m_lock(lock) {
            m_lock.lock();
        }

        ~spinlock_auto() noexcept {
            m_lock.unlock();
        }
    };
}

#endif //TFSPINLOCK_H
