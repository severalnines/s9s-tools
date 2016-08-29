/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

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

    private:
        int             m_referenceCounter;
        S9sString       m_hostName;
        int             m_port;
};
