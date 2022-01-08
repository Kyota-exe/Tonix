#include "Vector.h"

const uint64_t VECTOR_DEFAULT_CAPACITY = 4;

template<typename T>
Vector<T>::Vector()
{
    buffer = (T*)KMalloc(VECTOR_DEFAULT_CAPACITY * sizeof(T));
    capacity = VECTOR_DEFAULT_CAPACITY;
    length = 0;
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

        Serial::Printf("Old buffer: %x", (uint64_t)buffer);
        Serial::Printf("New buffer: %x", (uint64_t)newBuffer);

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
T& Vector<T>::operator[](uint64_t index)
{
    return buffer[index];
}

template class Vector<uint64_t>;