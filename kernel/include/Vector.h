#ifndef MISKOS_VECTOR_H
#define MISKOS_VECTOR_H

#include <stdint.h>
#include "Memory/Memory.h"
#include "Heap.h"
#include "Serial.h"

template <typename T> class Vector
{
private:
    T* buffer;
    uint64_t capacity;
    uint64_t length;
    bool initialized = false;

public:
    void Push(T value);
    void Remove(uint64_t index);
    uint64_t GetLength();

    typedef T* iterator;
    iterator begin();
    iterator end();

    T& operator[](uint64_t index);
    Vector<T>& operator=(const Vector<T>& newVector);

    Vector();
    Vector(const Vector<T>& original);
    ~Vector();
};

const uint64_t VECTOR_DEFAULT_CAPACITY = 4;

template<typename T>
Vector<T>::Vector()
{
    buffer = (T*)KMalloc(VECTOR_DEFAULT_CAPACITY * sizeof(T));
    capacity = VECTOR_DEFAULT_CAPACITY;
    length = 0;

    initialized = true;
}

template<typename T>
Vector<T>::Vector(const Vector<T> &original)
{
    capacity = original.capacity;
    length = original.length;
    buffer = (T*)KMalloc(length * sizeof(T));
    MemCopy(buffer, original.buffer, length);

    initialized = true;
}

template<typename T>
Vector<T>::~Vector()
{
    KFree(buffer);
}

template<typename T>
void Vector<T>::Push(T value)
{
    if (length == capacity)
    {
        capacity *= 2;
        T* newBuffer = (T*)KMalloc(capacity * sizeof(T));
        MemCopy(newBuffer, buffer, length * sizeof(T));
        KFree(buffer);

        buffer = newBuffer;
    }

    buffer[length] = value;
    length++;
}

template<typename T>
void Vector<T>::Remove(uint64_t index)
{
    length--;
    for (uint64_t i = index; i < length; ++i)
    {
        buffer[i] = buffer[i + 1];
    }
}

template<typename T>
uint64_t Vector<T>::GetLength()
{
    return length;
}

template<typename T>
typename Vector<T>::iterator Vector<T>::begin()
{
    return buffer;
}

template<typename T>
typename Vector<T>::iterator Vector<T>::end()
{
    return buffer + GetLength();
}

template<typename T>
T& Vector<T>::operator[](uint64_t index)
{
    return buffer[index];
}

template<typename T>
Vector<T>& Vector<T>::operator=(const Vector<T>& newVector)
{
    if (&newVector != this)
    {
        if (initialized) KFree(buffer);
        length = newVector.length;
        capacity = newVector.capacity;
        MemCopy(buffer, newVector.buffer, length);
    }
    return *this;
}

#endif
