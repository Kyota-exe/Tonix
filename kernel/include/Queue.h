#pragma once

#include "Vector.h"

template <typename T> class Queue : private Vector<T>
{
public:
    T Dequeue();
    void Enqueue(const T& value);

    using Vector<T>::GetLength;
    using Vector<T>::IsEmpty;
    using Vector<T>::operator=;
};

template <typename T>
T Queue<T>::Dequeue()
{
    return Vector<T>::Pop(0);
}

template <typename T>
void Queue<T>::Enqueue(const T& value)
{
    Vector<T>::Push(value);
}