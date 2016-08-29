/*
 * Copyright (C) 2016 severalnines.com
 */
#pragma once

class S9sRpcClientPrivate;

class S9sRpcClient
{
    public:
        S9sRpcClient();
        S9sRpcClient(const S9sRpcClient &orig);
        virtual ~S9sRpcClient();

        S9sRpcClient &operator=(const S9sRpcClient &rhs);

    private:
        S9sRpcClientPrivate *m_priv;
};

