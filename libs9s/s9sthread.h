/*
 * Copyright (C) 2011-2018 severalnines.com
 */
#pragma once

#include <pthread.h>

class S9sThread
{
    public:
        bool start();

    protected:
        enum State 
        {
            Created,
            Starting,
            Running,
            Stopping,
            Stopped
        };

        virtual int exec();
        virtual bool shouldStop() const;

    private:
        static void *threadEntryPoint(void *pointer);
        void run();
        
    private:
        pthread_t  m_thread;
        State      m_state;
        int        m_retval; 
};

