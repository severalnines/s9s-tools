/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include <stdlib.h>
#include "S9sString"
#include "S9sRpcReply"

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

        bool connect();
        void close();
        ssize_t write(const char *data, size_t length);
        ssize_t read(char *buffer, size_t bufSize);

    private:
        int             m_referenceCounter;
        int             m_socketFd;
        S9sString       m_hostName;
        int             m_port;
        S9sString       m_token;
        S9sString       m_errorString;
        S9sString       m_jsonReply;
        S9sRpcReply     m_reply;
        char           *m_buffer;
        size_t          m_bufferSize;
        size_t          m_dataSize;

        friend class S9sRpcClient;
};
