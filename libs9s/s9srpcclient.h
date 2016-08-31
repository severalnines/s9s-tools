/*
 * Copyright (C) 2016 severalnines.com
 */
#pragma once

#include "S9sString"
#include "S9sRpcReply"

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
    
        const S9sRpcReply &reply() const;
        S9sString errorString() const;
        
        /*
         * The executers that send an RPC request and receive an RPC reply from
         * the server.
         */
        bool getClusters();
        bool getJobInstances(const int clusterId);
        bool rollingRestart(const int clusterId);

    protected:
        int executeRequest(
                const S9sString &uri,
                const S9sString &payload);

    private:
        S9sRpcClientPrivate *m_priv;
};

