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

        bool setHost(
                const int             clusterId,
                const S9sVariantList &hostNames,
                const S9sVariantMap  &properties);

        bool getJobInstances(const int clusterId);

        bool getJobInstance(const int clusterId, const int jobId);
        
        bool getJobLog(
                const int clusterId, 
                const int jobId,
                const int limit   = 0,
                const int offset  = 0);

        bool rollingRestart(const int clusterId);

        bool createGaleraCluster(
                const S9sVariantList &hostNames,
                const S9sString      &osUserName,
                const S9sString      &vendor,
                const S9sString      &mySqlVersion,
                bool                  uninstall);

        bool createMySqlReplication(
                const S9sVariantList &hostNames,
                const S9sString      &osUserName,
                const S9sString      &vendor,
                const S9sString      &mySqlVersion,
                bool                  uninstall);

        bool addNode(const S9sVariantList &hostNames);
        bool removeNode(const S9sVariantList &hostNames);

    protected:
        bool executeRequest(
                const S9sString &uri,
                const S9sString &payload);

    private:
        S9sRpcClientPrivate *m_priv;
};

