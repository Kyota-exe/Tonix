#include "Spinlock.h"

void Spinlock::Acquire()
{
    while (__atomic_test_and_set(&locked, __ATOMIC_ACQUIRE))
    {
        asm volatile("pause");
    }
}

void Spinlock::Release()
{
    __atomic_clear(&locked, __ATOMIC_RELEASE);
}