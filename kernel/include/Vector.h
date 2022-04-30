#pragma once

#include <stdint.h>
#include "Assert.h"

template <typename T> class Vector
{
private:
    T* buffer;
    uint64_t capacity;
    uint64_t length;

public:
    uint64_t Push(const T& value);
    T Pop();
    T Pop(uint64_t index);
    uint64_t GetLength() const;
    bool IsEmpty() const;

    T* begin();
    T* end();
    const T* begin() const;
    const T* end() const;

    T& Get(uint64_t index);
    const T& Get(uint64_t index) const;
    T& GetLast();
    const T& GetLast() const;
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
uint64_t Vector<T>::GetLength() const
{
    return length;
}

template <typename T>
bool Vector<T>::IsEmpty() const
{
    return GetLength() == 0;
}

template <typename T>
T* Vector<T>::begin()
{
    return buffer;
}

template <typename T>
T* Vector<T>::end()
{
    return buffer + GetLength();
}

template <typename T>
const T* Vector<T>::begin() const
{
    return buffer;
}

template <typename T>
const T* Vector<T>::end() const
{
    return buffer + GetLength();
}

template <typename T>
T& Vector<T>::Get(uint64_t index)
{
    Assert(index < GetLength());
    return buffer[index];
}

template <typename T>
const T& Vector<T>::Get(uint64_t index) const
{
    Assert(index < GetLength());
    return buffer[index];
}

template <typename T>
T& Vector<T>::GetLast()
{
    return Get(length - 1);
}

template <typename T>
const T& Vector<T>::GetLast() const
{
    return Get(length - 1);
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
