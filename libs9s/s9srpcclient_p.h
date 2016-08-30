/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include <stdlib.h>
#include "S9sString"
#include "S9sVariantMap"

class S9sRpcClientPrivate
{
    public:
        S9sRpcClientPrivate();
        ~S9sRpcClientPrivate();

        void ref();
        int unRef();

    private:
        void clearBuffer();
        void ensureHasBuffer(size_t size);

        int connectSocket();
        void closeSocket(int socketFd);
        ssize_t writeSocket(int socketFd, const char *data, size_t length);
        ssize_t readSocket(int socketFd, char *buffer, size_t bufSize);

    private:
        int             m_referenceCounter;
        S9sString       m_hostName;
        int             m_port;
        S9sString       m_token;
        S9sString       m_jsonReply;
        S9sVariantMap   m_reply;
        char           *m_buffer;
        size_t          m_bufferSize;
        size_t          m_dataSize;

        friend class S9sRpcClient;
};
