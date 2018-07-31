/*
 * Copyright (C) 2011-2018 severalnines.com
 */
#include "s9sthread.h"

#include <unistd.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"


/**
 * \returns true if the thread was successfully started, false on an error
 *
 * This method should always return true, creating a new thread should not fail.
 */
bool
S9sThread::start()
{
    S9S_DEBUG("");
    if (pthread_create(&m_thread, NULL, S9sThread::threadEntryPoint, this))
    {
        S9S_WARNING("pthread_create() failed: %m");
        return false;
    }

    return true;
}

int
S9sThread::exec()
{
    int retval = 0;

    do
    {
        S9S_DEBUG("tick");
        ++retval;
        usleep(1000000);
    } while (!shouldStop());

    return retval;
}

bool
S9sThread::shouldStop() const
{
    return m_state == Stopping;
}

void
S9sThread::run()
{
    m_state = Running;
    m_retval = exec();
    m_state = Stopping;
}

void *
S9sThread::threadEntryPoint(
        void *pointer)
{
    S9sThread *self = (S9sThread *) pointer;
    self->run();
    return NULL;
}

