#pragma once

#include "Vector.h"

template <typename T> class Queue
{
public:
    T Dequeue();
    void Enqueue(const T& value);
    bool IsEmpty() const;
private:
    Vector<T> vector;
};

template <typename T>
T Queue<T>::Dequeue()
{
    return vector.Pop(0);
}

template <typename T>
void Queue<T>::Enqueue(const T& value)
{
    vector.Push(value);
}

template <typename T>
bool Queue<T>::IsEmpty() const
{
    return vector.IsEmpty();
}
