/*
 * Severalnines Tools
 * Copyright (C) 2016  Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * S9sTools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S9sTools. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <vector>
#include <algorithm>
#include <assert.h>

template <typename T>
class S9sVector : public std::vector<T>
{
    public:
        bool contains(const T &item) const;

        S9sVector<T> &operator=(const std::vector<T> &rhs);
        S9sVector<T> &operator<<(const T &item);
        S9sVector<T> &operator<<(const S9sVector<T> &toInsert);

        void append(const S9sVector<T> &toInsert);
        void append(const std::vector<T> &toInsert);

        void copyElements(
                const S9sVector<T> &toCopy,
                const unsigned int from,
                const unsigned int to);

        void copyElements(
                const std::vector<T> &toCopy,
                const unsigned int from,
                const unsigned int to );

        S9sVector<T> unique() const;
        void sort();
        void reverse();

        T takeLast();
        T takeFirst();

        void limit(const int sizeLimit);
};

template <typename T>
S9sVector<T> &
S9sVector<T>::operator=(
        const std::vector<T> &rhs)
{
    if (&rhs == this)
        return *this;

    this->clear();

    for (unsigned int idx = 0u; idx < rhs.size(); ++idx)
        this->push_back(rhs[idx]);

    return *this;
}

/**
 * \returns true if the vector contains the given element.
 */
template <typename T>
bool S9sVector<T>::contains(
        const T &item) const
{
    for (typename std::vector<T>::const_iterator it = this->begin(); it != this->end (); it++) 
    {
        if (item == *it)
            return true;
    }

    return false;
}

/**
 * Appends one elements to the end of the vector.
 */
template <typename T>
S9sVector<T> &S9sVector<T>::operator<<(
        const T &item)
{
    this->push_back(item);
    return *this;
}

template <typename T>
S9sVector<T> &S9sVector<T>::operator<<(
        const S9sVector<T> &toInsert)
{
    this->insert(this->end(), toInsert.begin(), toInsert.end());
    return *this;
}

/**
 * Appends several elements to the end of the vector.
 */
template <typename T>
void S9sVector<T>::append(
        const S9sVector<T> &toInsert)
{
    this->insert(this->end(), toInsert.begin(), toInsert.end());
}

/**
 * Appends several elements to the end of the vector.
 */
template <typename T>
void S9sVector<T>::append(
        const std::vector<T> &toInsert)
{
    this->insert(this->end(), toInsert.begin(), toInsert.end());
}

/**
 * Removes and returns the last element of the vector.
 */
template <typename T>
T S9sVector<T>::takeLast()
{
    assert(!this->empty());

    T retval = this->back();
    this->pop_back();
    return retval;
}

template <typename T>
T S9sVector<T>::takeFirst()
{
    assert(!this->empty());

    T retval = this->front();
    this->erase(this->begin());
    return retval;
}

/**
 *  several elements to the end of the vector.
 */
template <typename T>
void S9sVector<T>::copyElements(
        const std::vector<T> &toInsert,
        const unsigned int from,
        const unsigned int to)
{
    this->insert(this->end(), toInsert.begin()+from, to>toInsert.size() ? toInsert.end() : toInsert.begin() + to );
}

template <typename T>
void S9sVector<T>::copyElements(
        const S9sVector<T> &toInsert,
        const unsigned int from,
        const unsigned int to)
{
    this->insert(this->end(), toInsert.begin()+from, to>toInsert.size() ? toInsert.end() : toInsert.begin() + to );
}

/**
 * \returns a new S9sVector which doesn't contain any duplicates,
 *   so every item is unique in the new vector
 */
template <typename T>
S9sVector<T> S9sVector<T>::unique() const
{
    S9sVector<T> retval;

    for (typename std::vector<T>::iterator it = this->begin(); it != this->end(); ++it)
    {
        if (retval.contains (*it))
            continue;
        retval.push_back (*it);
    }

    return retval;
}

/**
 * \param sizeLimit the maximum number of items this list should hold or -1
 *
 * This function will drop items until it holds no more items than the size
 * limit.
 *
 * FIXME: maybe we should store this limit in the object and then we could
 * investigate the size before trying to add items.
 */
template <typename T>
void
S9sVector<T>::limit(
        const int sizeLimit)
{
    if (sizeLimit < 0)
        return;

    while ((int)this->size() > sizeLimit)
        this->pop_back();
}

/**
 * Sorts the list in ascending alphabetical order.
 */
template <typename T>
void
S9sVector<T>::sort()
{
    std::sort(this->begin(), this->end());
}

/**
 * Reverse order. 
 */
template <typename T>
void
S9sVector<T>::reverse()
{
    std::reverse(this->begin(), this->end());
}

