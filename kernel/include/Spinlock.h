#pragma once

class Spinlock
{
public:
    void Acquire();
    void Release();
private:
    bool locked = false;
};