#ifndef SIMPLE_SPINLOCK_H
#define SIMPLE_SPINLOCK_H

#include <atomic>

class Spinlock
{
public:
    Spinlock()
    {
        flag.clear();
    }

    Spinlock(const Spinlock &tm) = delete;
    Spinlock(Spinlock &&tm) = delete;
    Spinlock& operator=(const Spinlock &tm) = delete;
    Spinlock& operator=(Spinlock && tm) = delete;

    void lock()
    {
        while(!flag.test_and_set(std::memory_order_acquire)) { }
    }

    bool tryLock()
    {
        return flag.test_and_set(std::memory_order_acquire) == false;
    }

    void unlock()
    {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag;
};

#endif // SIMPLE_SPINLOCK_H

