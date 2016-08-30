/*
 * Copyright (C) 2016 severalnines.com
 */
#pragma once

#include "S9sString"

class S9sRpcClientPrivate;

class S9sRpcClient
{
    public:
        S9sRpcClient();
        S9sRpcClient(
                const S9sString &hostName,
                const int        port,
                const S9sString &token);
        S9sRpcClient(const S9sRpcClient &orig);

        virtual ~S9sRpcClient();

        S9sRpcClient &operator=(const S9sRpcClient &rhs);

        void getClusters();

    protected:
        int executeRequest(
                const S9sString &uri,
                const S9sString &payload);

    private:
        S9sRpcClientPrivate *m_priv;
};

