/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include <cstdlib>
#include <openssl/ssl.h>

#include "s9sstring.h"
#include "s9srpcreply.h"
#include "s9svariantmap.h"
#include "s9scontroller.h"
#include "s9srpcclient.h"

class S9sRpcClientPrivate
{
    public:
        S9sRpcClientPrivate();
        ~S9sRpcClientPrivate();

        void ref();
        int unRef();

        void rememberRedirect();
        bool loadRedirect();

        void setConnectFailed(
                const S9sString  &hostName, 
                const int         port);

        bool tryNextHost(S9s::Redirect redirect);

        void printBuffer(const S9sString &title);

        bool hasCompleteJSon() const;
        S9sString getCompleteJSon() const;
        bool skipRecord();

    private:
        void clearBuffer();
        void ensureHasBuffer(size_t size);

        bool connect(S9s::Redirect redirect = S9s::AllowRedirect);
        void close();
        ssize_t write(const char *data, size_t length);
        ssize_t read(char *buffer, size_t bufSize);

        void setBuffer(S9sString &content, int additionalSize = 0);

        void parseHeaders();
        S9sString cookieHeaders() const;
        S9sString serverVersionString() const;

    private:
        int             m_referenceCounter;
        ulonglong       m_requestId;
        int             m_socketFd;
        S9sString       m_hostName;
        int             m_port;
        S9sString       m_path;
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
        bool            m_authenticated;
        
        S9sVariantList  m_controllers;
        S9sVector<S9sController> m_servers;
        friend class S9sRpcClient;
};

