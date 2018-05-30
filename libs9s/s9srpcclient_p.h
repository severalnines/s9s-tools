/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include <cstdlib>
#include <openssl/ssl.h>

#include "S9sString"
#include "S9sRpcReply"
#include "S9sVariantMap"
#include "s9srpcclient.h"

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

        void parseHeaders();
        S9sString cookieHeaders() const;
        S9sString serverVersionString() const;

    private:
        int             m_referenceCounter;
        ulonglong       m_requestId;
        int             m_socketFd;
        S9sString       m_hostName;
        int             m_port;
        bool            m_useTls;
        S9sString       m_errorString;
        S9sString       m_jsonReply;
        S9sRpcReply     m_reply;
        char           *m_buffer;
        size_t          m_bufferSize;
        size_t          m_dataSize;
        SSL_CTX        *m_sslContext;
        SSL            *m_ssl;
        S9sVariantMap   m_cookies;
        S9sString       m_serverHeader;

        S9sJSonHandler  m_callbackFunction;
        void           *m_callbackUserData;

        friend class S9sRpcClient;
};

