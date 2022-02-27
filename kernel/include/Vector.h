#pragma once

#include <stdint.h>
#include "Heap.h"
#include "Assert.h"

template <typename T> class Vector
{
public:
    T* buffer;
    uint64_t capacity;
    uint64_t length;

public:
    uint64_t Push(const T& value);
    T Pop();
    T Pop(uint64_t index);
    bool Contains(const T& value);
    uint64_t GetLength();

    typedef T* iterator;
    iterator begin();
    iterator end();

    T& Get(uint64_t index);
    T& operator[](uint64_t index);
    Vector<T>& operator=(const Vector<T>& newVector);

    Vector();
    Vector(const Vector<T>& original);
    ~Vector();
};

const uint64_t VECTOR_DEFAULT_CAPACITY = 1;

template <typename T>
Vector<T>::Vector()
{
    buffer = new T[VECTOR_DEFAULT_CAPACITY];
    capacity = VECTOR_DEFAULT_CAPACITY;
    length = 0;
}

template <typename T>
Vector<T>::Vector(const Vector<T>& original)
{
    capacity = original.capacity;
    length = original.length;
    buffer = new T[length];

    for (uint64_t i = 0; i < length; ++i)
    {
        buffer[i] = original.buffer[i];
    }
}

template <typename T>
Vector<T>::~Vector()
{
    delete[] buffer;
}

template <typename T>
uint64_t Vector<T>::Push(const T& value)
{
    if (length == capacity)
    {
        capacity *= 2;
        T* newBuffer = new T[capacity];

        for (uint64_t i = 0; i < length; ++i)
        {
            newBuffer[i] = buffer[i];
        }

        delete[] buffer;
        buffer = newBuffer;
    }

    uint64_t index = length;
    buffer[index] = value;
    length++;

    return index;
}

template <typename T>
T Vector<T>::Pop()
{
    Assert(length > 0);

    T value = buffer[length - 1];
    length--;

    return value;
}

template <typename T>
T Vector<T>::Pop(uint64_t index)
{
    Assert(index < length);

    T value = buffer[index];
    length--;

    for (uint64_t i = index; i < length; ++i)
    {
        buffer[i] = buffer[i + 1];
    }

    return value;
}

template <typename T>
bool Vector<T>::Contains(const T& value)
{
    for (uint64_t i = 0; i < length; ++i)
    {
        if (buffer[i] == value) return true;
    }
    return false;
}

template <typename T>
uint64_t Vector<T>::GetLength()
{
    return length;
}

template <typename T>
typename Vector<T>::iterator Vector<T>::begin()
{
    return buffer;
}

template <typename T>
typename Vector<T>::iterator Vector<T>::end()
{
    return buffer + GetLength();
}

template <typename T>
T& Vector<T>::operator[](uint64_t index)
{
    Assert(index < GetLength());
    return buffer[index];
}

template <typename T>
T& Vector<T>::Get(uint64_t index)
{
    Assert(index < GetLength());
    return buffer[index];
}

template <typename T>
Vector<T>& Vector<T>::operator=(const Vector<T>& newVector)
{
    if (&newVector != this)
    {
        delete[] buffer;
        length = newVector.length;
        capacity = newVector.capacity;
        buffer = new T[capacity];
        for (uint64_t i = 0; i < length; ++i)
        {
            buffer[i] = newVector.buffer[i];
        }
    }
    return *this;
}
