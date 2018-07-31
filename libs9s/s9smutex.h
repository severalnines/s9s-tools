/*
 * Copyright (C) 2011-2018 severalnines.com
 */
#pragma once
#include <pthread.h>

class S9sMutex
{
    public:
        S9sMutex();
        ~S9sMutex();

        void lock();
        void unlock();

    private:
        pthread_mutex_t     m_mutex;
        pthread_mutexattr_t m_attrs;
};
