/*
 * Copyright (C) 2011-2018 severalnines.com
 */
#include "s9smutex.h"


S9sMutex::S9sMutex()
{
    pthread_mutexattr_init(&m_attrs);
    pthread_mutex_init(&m_mutex, &m_attrs);
}
 
S9sMutex::~S9sMutex()
{
    pthread_mutex_destroy(&m_mutex);
    pthread_mutexattr_destroy(&m_attrs);
}

void
S9sMutex::lock()
{
    pthread_mutex_lock(&m_mutex);
}

void
S9sMutex::unlock()
{
    pthread_mutex_unlock(&m_mutex);
}

