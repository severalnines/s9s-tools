/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include <stdlib.h>
#include "S9sString"

class S9sRpcClientPrivate
{
    public:
        S9sRpcClientPrivate();
        ~S9sRpcClientPrivate();

        void ref();
        int unRef();

    private:
        int connectSocket();
        void closeSocket(int socketFd);
        ssize_t writeSocket(int socketFd, const char *data, size_t length);

    private:
        int             m_referenceCounter;
        S9sString       m_hostName;
        int             m_port;
        S9sString       m_token;

        friend class S9sRpcClient;
};
