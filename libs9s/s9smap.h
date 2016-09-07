/* 
 * Copyright (C) 2011-2015 severalnines.com
 */
#pragma once

#include <map>
#include <vector>
#include <string>

#include "S9sGlobal"
#include "S9sString"
#include "S9sVector"

template <typename Key, typename T>
class S9sMap : public std::map<Key, T>
{
    public:
        bool contains(const Key &key) const;
        S9sVector<Key> keys() const;
#if 0
        static S9sMap<Key, T> fromVector(const std::vector<Key> &theVector);
        static S9sMap<Key, T> fromVector(const CmonStringList &theVector);
#endif
};

/**
 * \returns true if the map contains an element with the given key
 *
 * One can't be careful enough with checking the maps for keys. The [] 
 * operator would add a new element created by the default constructor, so it is
 * not very good for testing if the key can be found. A simple way to check this
 * is to use this contains() method. So instead of

 <code>
 if (!v[key].isNull())
    v[key].something();
 </code>

 * use this

 <code>
 if (v.contains(key))
    v[key].something();
 </code>

 */
template <typename Key, typename T>
inline bool S9sMap<Key, T>::contains(
        const Key &key) const
{
    return this->find(key) != this->end();
}


/**
 * \returns a list that holds all the keys from the map
 */
template <typename Key, typename T>
S9sVector<Key> S9sMap<Key, T>::keys() const
{
    S9sVector<Key> retval;

    for (typename std::map<Key, T>::const_iterator it = this->begin(); it != this->end(); ++it) 
    {
        retval.push_back(it->first);
    }

    return retval;
}


#if 0
/**
 * Handy little method to convert a vector to a map. use like this:
 <code>
std::vector<string>        theList;
S9sMap<std::string, bool> theMap;
 ...
theMap = S9sMap<std::string, bool>::fromVector(theList);
 </code>
 */
template <typename Key, typename T>
S9sMap<Key, T> S9sMap<Key, T>::fromVector(
        const std::vector<Key> &theVector)
{
    S9sMap<Key, T> retval;

    for (uint idx = 0; idx < theVector.size(); ++idx)
        retval[theVector[idx]] = T();

    return retval;
}
#endif

#if 0
template <typename Key, typename T>
S9sMap<Key, T> S9sMap<Key, T>::fromVector(
        const S9sStringList &theVector)
{
    S9sMap<Key, T> retval;

    for (uint idx = 0; idx < theVector.size(); ++idx)
        retval[theVector[idx]] = T();

    return retval;
}
#endif

