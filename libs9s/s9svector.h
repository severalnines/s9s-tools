/*
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include <vector>
#include <assert.h>

template <typename T>
class S9sVector : public std::vector<T>
{
    public:
        bool contains(const T &item) const;

        S9sVector<T> &operator=(const std::vector<T> &rhs);
        S9sVector<T> &operator=(std::vector<T> && rhs);
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

template <typename T>
S9sVector<T> &S9sVector<T>::operator=(
        std::vector<T> && rhs)
{
    std::vector<T>::operator=((std::vector<T> &&)rhs);
    return *this;
}

/**
 * \returns true if the vector contains the given element.
 */
template <typename T>
bool S9sVector<T>::contains(
        const T &item) const
{
    for (auto it = this->begin (); it != this->end (); it++) 
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

    for (auto it = this->begin(); it != this->end(); ++it)
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

