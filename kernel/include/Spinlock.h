#pragma once

#include <stdint.h>

class Spinlock
{
public:
    void Acquire();
    void Release();
private:
    uint32_t nextTicket;
    uint32_t servingTicket;
};