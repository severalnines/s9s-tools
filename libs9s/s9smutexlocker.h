/*
 * Copyright (C) 2011-2018 severalnines.com
 */
#pragma once
#include "s9smutex.h"

class S9sMutexLocker
{
    public:
        S9sMutexLocker (S9sMutex &mutex); 
        virtual ~S9sMutexLocker();

    private:
        S9sMutex   &m_mutex;
};

