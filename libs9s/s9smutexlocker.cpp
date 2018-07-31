/*
 * Copyright (C) 2011-2018 severalnines.com
 */
#include "s9smutexlocker.h"

S9sMutexLocker::S9sMutexLocker (
        S9sMutex        &mutex) : 
    m_mutex (mutex)
{
    m_mutex.lock();
}

/**
 *
 */
S9sMutexLocker::~S9sMutexLocker()
{
    m_mutex.unlock();
}
