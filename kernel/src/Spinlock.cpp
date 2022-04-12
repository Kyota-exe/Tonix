#include "Spinlock.h"

void Spinlock::Acquire()
{
    auto ticket = __atomic_fetch_add(&nextTicket, 1, __ATOMIC_RELAXED);
    while (__atomic_load_n(&servingTicket, __ATOMIC_ACQUIRE) != ticket);
}

void Spinlock::Release()
{
    auto current = __atomic_load_n(&servingTicket, __ATOMIC_RELAXED);
    __atomic_store_n(&servingTicket, current + 1, __ATOMIC_RELEASE);
}