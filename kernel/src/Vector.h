#ifndef MISKOS_VECTOR_H
#define MISKOS_VECTOR_H

#include <stdint.h>
#include "Heap.h"

template <typename T> class Vector
{
private:
    T* buffer;
    uint64_t capacity;
    uint64_t length;

public:
    void Push(T value);
    void Remove(uint64_t index);
    uint64_t GetLength();
    T& operator[](uint64_t index);
    Vector();
    ~Vector();
};

#endif
