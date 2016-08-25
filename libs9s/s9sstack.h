/* 
 * Copyright (C) 2011-2015 severalnines.com
 */
#pragma once

#include <vector>
#include <assert.h>

template <typename T>
class S9sStack : public std::vector<T>
{
    public:
        void push(const T &item);
        T pop();
        T &top();
        const T &top() const;
};

template <typename T>
T
S9sStack<T>::pop()
{
    assert(!this->empty());

    T retval = this->front();
    this->erase(this->begin());
    return retval;
}

template <typename T>
T &
S9sStack<T>::top()
{
    assert(!this->empty());

    return this->front();
}

template <typename T>
const T &
S9sStack<T>::top() const
{
    assert(!this->empty());

    return this->front();
}

template <typename T>
void 
S9sStack<T>::push(
        const T &item)
{
    this->insert(this->begin(), item);
}


