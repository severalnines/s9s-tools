/*
 * Severalnines Tools
 * Copyright (C) 2017  Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * s9s-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with s9s-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9srpcclient.h"
#include "s9srpcclient_p.h"

#include "S9sOptions"
#include "S9sUser"
#include "S9sNode"
#include "S9sAccount"
#include "S9sRsaKey"
#include "S9sDateTime"
#include "S9sFile"

#include <cstring>
#include <cstdio>

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

#define READ_SIZE 512

/**
 * Default constructor.
 */
S9sRpcClient::S9sRpcClient() :
    m_priv(new S9sRpcClientPrivate)
{
}

/**
 * \param hostName the name if the host where the Cmon controller accepts
 *   requests.
 * \param port the port where the Cmon controller accepts requests.
 * \param useTls if client must initiate TLS encryption to the server.
 *
 */
S9sRpcClient::S9sRpcClient(
        const S9sString &hostName,
        const int        port,
        const bool       useTls) :
    m_priv(new S9sRpcClientPrivate)
{
    m_priv->m_hostName = hostName;
    m_priv->m_port     = port;
    m_priv->m_useTls   = useTls;
}


/**
 * Copy constructor. Nothing to see here.
 */
S9sRpcClient::S9sRpcClient (
		const S9sRpcClient &orig)
{
	m_priv = orig.m_priv;

	if (m_priv) 
		m_priv->ref ();
}

/**
 * Normal destructor. 
 */
S9sRpcClient::~S9sRpcClient()
{
	if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
	}
}

/**
 * Assignment operator to utilize the implicit sharing.
 */
S9sRpcClient &
S9sRpcClient::operator= (
		const S9sRpcClient &rhs)
{
	if (this == &rhs)
		return *this;

	if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
	}

	m_priv = rhs.m_priv;
	if (m_priv) 
    {
		m_priv->ref ();
	}

	return *this;
}

bool
S9sRpcClient::hasPrivateKey() const
{
    S9sOptions    *options = S9sOptions::instance();
    S9sFile        keyFile(options->privateKeyPath());

    if (options->userName().empty())
        return false;
 
    return keyFile.exists();
}

bool
S9sRpcClient::canAuthenticate(
        S9sString &reason) const
{
    S9sOptions    *options = S9sOptions::instance();

    // No authentication without a username.
    if (options->userName().empty())
    {
        reason = "No user name set.";
        return false;
    }

    // It is possible with a password...
    if (!options->password().empty())
        return true;

    // or a key.
    if (hasPrivateKey())
        return true;

    reason.sprintf(
            "No password and no RSA key for user %s.", 
            STR(options->userName()));

    return false;
}

bool
S9sRpcClient::needToAuthenticate() const
{
    S9sOptions    *options = S9sOptions::instance();

    // Creating a new user is possible without authentication through the pipe.
    if (options->isUserOperation() && options->isCreateRequested())
        return false;

    return true;
}

/**
 * \returns the reply that received from the controller.
 *
 * The reply the controller sends is a JSON string which is parsed by the
 * S9sRpcClient and presented here as an S9sVariantMap (S9sRpcReply that
 * inherits S9sVariantMap to be more precise).
 */
const S9sRpcReply &
S9sRpcClient::reply() const
{
    return m_priv->m_reply;
}

/**
 * Takes the request status from the reply and sets the exit code of the
 * program accordingly. If the request status is "ok" the exit code is not going
 * to be changed.
 */
void
S9sRpcClient::setExitStatus() 
{
    S9sRpcReply::ErrorCode errorCode = reply().requestStatus();

    if (errorCode != S9sRpcReply::Ok)
    {
        S9sOptions *options = S9sOptions::instance();

        switch (errorCode)
        {
            case S9sRpcReply::Ok:
                break;

            case S9sRpcReply::InvalidRequest:
                options->setExitStatus(S9sOptions::Failed);
                break;

            case S9sRpcReply::ObjectNotFound:
                options->setExitStatus(S9sOptions::NotFound);
                break;

            case S9sRpcReply::TryAgain:
                options->setExitStatus(S9sOptions::Failed);
                break;

            case S9sRpcReply::ClusterNotFound:
                options->setExitStatus(S9sOptions::NotFound);
                break;

            case S9sRpcReply::UnknownError:
                options->setExitStatus(S9sOptions::Failed);
                break;

            case S9sRpcReply::AccessDenied:
                options->setExitStatus(S9sOptions::AccessDenied);
                break;

            case S9sRpcReply::AuthRequired:
                options->setExitStatus(S9sOptions::Failed);
                break;
        }
    }
}

/**
 * \returns the human readable error string stored in the object.
 */
S9sString 
S9sRpcClient::errorString() const
{
    return m_priv->m_errorString;
}

/**
 * Prints the messages from the reply, prints the default message if there are
 * no messages in the reply.
 */
void
S9sRpcClient::printMessages(
        const S9sString &defaultMessage,
        bool             success)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRpcReply    rpcReply;

    rpcReply = reply();
    if (success)
    {
        rpcReply.printMessages(defaultMessage);
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(rpcReply.toString()));
        else
            PRINT_ERROR("%s", STR(errorString()));
    }
}

void
S9sRpcClient::printServerRegistered(
        bool success)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRpcReply    rpcReply;

    rpcReply = reply();
    if (success)
    {
        rpcReply.printMessages("Registered.");
        #if 0
        printf("  Processors\n");
        rpcReply.printProcessors("    ");
        printf("  Memory\n");
        rpcReply.printMemoryBanks("    ");
        #endif
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(rpcReply.toString()));
        else
            PRINT_ERROR("%s", STR(errorString()));
    }
}


bool
S9sRpcClient::authenticate()
{
    S9sOptions    *options = S9sOptions::instance();

    if (options->hasPassword())
        return authenticateWithPassword();

    return authenticateWithKey();
}

bool 
S9sRpcClient::authenticateWithPassword()
{
    S9S_DEBUG(" ");
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantMap  request;
    S9sString      uri = "/v2/auth";
    bool           retval;

    request["operation"]    = "authenticateWithPassword";
    request["user_name"]    = options->userName();
    request["password"]     = options->password();
    
    retval = executeRequest(uri, request);
    if (!retval)
        return false;
    
    return reply().isOk();
}

/**
 * Does the authentication with the private key.
 */
bool
S9sRpcClient::authenticateWithKey()
{
    S9S_DEBUG(" ");
    S9sOptions    *options = S9sOptions::instance();
    S9sRsaKey      rsa;
    S9sString      uri = "/v2/auth";
    S9sVariantMap  request;
    bool           retval;
    S9sString      privKeyPath = options->privateKeyPath();

    S9S_DEBUG(" privKeyPath : %s", STR(privKeyPath));

    if (privKeyPath.empty())
    {
        S9S_WARNING("Private key not specified.");

        m_priv->m_errorString =
                "Private key not specified, authentication is not possible.";
        return false;
    }

    if (!rsa.loadKeyFromFile(privKeyPath))
    {
        S9S_WARNING(
                "Could not load user private key: %s",
                STR(privKeyPath));

        m_priv->m_errorString.sprintf (
                "Could not load user private key: %s",
                STR(privKeyPath));
        return false;
    }

    /*
     * First request.
     */
    request = S9sVariantMap();
    request["operation"]    = "authenticate";
    request["user_name"]    = options->userName();

    retval = executeRequest(uri, request);
    if (!retval)
    {
        S9S_DEBUG("request failed: %s", STR(reply().toString()));
        return false;
    }

    S9sRpcReply loginReply = reply();
    S9sString signature;
    S9sString challenge = loginReply["challenge"].toString();

    // create an RSA-SHA256 signature using user's privkey
    S9S_DEBUG("  privKeyPath : %s", STR(privKeyPath));
    rsa.signRsaSha256(challenge, signature);
    S9S_DEBUG("  challenge   : %s", STR(challenge));
    S9S_DEBUG("  signature   : %s", STR(signature));

    /*
     * Second request.
     */
    request = S9sVariantMap();
    request["operation"]    = "authenticateResponse";
    request["signature"]    = signature;

    retval = executeRequest(uri, request);
    if (!retval)
        return false;

    /*
     * If reply doesn't contain an error and we are ok, auth succeed
     */
    m_priv->m_errorString = reply().errorString ();
    return reply().isOk();
}

/**
 * \param clusterId The ID of the cluster for which the information is
 *   requested.
 *
 * The method that sends the "getAllClusterInfo" RPC request and reads the
 * reply. This function requests for information about only one cluster
 * referenced by the cluster ID.
 */
bool
S9sRpcClient::getCluster(
        const S9sString  &clusterName, 
        const int         clusterId)
{
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]       = "getClusterInfo";
    request["with_hosts"]      = true;
    request["with_sheet_info"] = true;

    if (S9S_CLUSTER_ID_IS_VALID(clusterId))
        request["cluster_id"]   = clusterId;

    if (!clusterName.empty())
        request["cluster_name"] = clusterName;
    
    retval = executeRequest(uri, request);
    
    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * The method that sends the "getAllClusterInfo" RPC request and reads the
 * reply.
 */
bool
S9sRpcClient::getClusters()
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sString      clusterName = options->clusterName();
    int            clusterId   = options->clusterId();
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;
    bool           retval;
   
    if (options->hasClusterIdOption())
        return getCluster(clusterName, clusterId);
    else if (options->hasClusterNameOption())
        return getCluster(clusterName, clusterId);

    request["operation"]       = "getAllClusterInfo";
    request["with_hosts"]      = true;
    request["with_sheet_info"] = true;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * Gets the list of databases from the controller. Getting the databases here is
 * basically getting the clusters, but with requesting the databases of the
 * cluster too.
 */
bool
S9sRpcClient::getDatabases()
{
    S9sOptions    *options     = S9sOptions::instance();
    S9sString      operation   = "getAllClusterInfo";
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;
    bool           retval;
   
    if (options->hasClusterIdOption())
    {
        request["operation"]       = "getClusterInfo";
        request["cluster_id"]      = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["operation"]       = "getClusterInfo";
        request["cluster_name"]    = options->clusterName();
    } else {
        request["operation"]       = "getAllClusterInfo";
    }

    request["with_databases"]  = true;
    
    if (options->isRefreshRequested())
        request["refresh_now"] = true;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This method id used to get the Cmon Directory Tree from the controller.
 */
bool
S9sRpcClient::getTree()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/tree";
    S9sVariantMap  request;
    
    request["operation"]       = "getTree";

    if (options->nExtraArguments() > 0)
        request["path"] = options->extraArgument(0u);

    if (options->isRefreshRequested())
        request["refresh_now"] = true;

    return executeRequest(uri, request);
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * The method that sends the "getConfig" RPC request and reads the
 * reply. The "getConfig" returns the parsed version of the configuration for a
 * given node.
 */
bool
S9sRpcClient::getConfig(
        const S9sVariantList &hosts)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/config/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "getConfig";
    if (hosts.size() == 1u)
    {
        S9sNode node = hosts[0].toNode();

        request["hostname"] = node.hostName();

        if (node.hasPort())
            request["port"] = node.port();
    } else {
        PRINT_ERROR("getConfig only implemented for one host.");
        return false;
    }

    if (options->clusterId() > 0)
        request["cluster_id"] = options->clusterId();

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function is for changing the configuration through the controller for 
 * one node.
 */
bool
S9sRpcClient::setConfig(
        const S9sVariantList &hosts)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/config/";
    S9sVariantMap  request;
    S9sVariantList optionList;
    S9sVariantMap  optionMap;
    bool           retval;

    request["operation"]  = "setConfig";
    if (hosts.size() == 1u)
    {
        S9sNode node = hosts[0].toNode();

        request["hostname"] = node.hostName();

        if (node.hasPort())
            request["port"] = node.port();
    } else {
        PRINT_ERROR("setConfig only implemented for one host.");
        return false;
    }

    if (options->clusterId() > 0)
        request["cluster_id"] = options->clusterId();
    
    // 
    // The configuration value: here it is implemented for one value.
    //
    optionMap["name"]  = options->optName();
    optionMap["value"] = options->optValue();

    if (!options->optGroup().empty())
        optionMap["group"] = options->optGroup();

    optionList << optionMap;

    request["configuration"] = optionList;

    retval = executeRequest(uri, request);
    return retval;
}

bool
S9sRpcClient::ping()
{
    S9sDateTime    now = S9sDateTime::currentDateTime();
    S9sString      timeString = now.toString(S9sDateTime::TzDateTimeFormat);
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]       = "ping";
    request["request_created"] = timeString;
    
    retval = executeRequest(uri, request);

    return retval;
}

bool 
S9sRpcClient::setHost()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  hostNames;
    S9sVariantMap   properties;

    hostNames = options->nodes();
    if (hostNames.empty())
    {
        PRINT_ERROR(
                "Node list is empty while setting node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }
    
    properties = options->propertiesOption();
    if (properties.empty())
    {
        PRINT_ERROR(
                "Properties not provided while setting node.\n"
                "Use the --properties command line option to provide"
                " properties."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    return setHost(hostNames, properties);
}



/**
 * \param hosts The list of hosts to change (currently only one host is
 *   supported).
 * \param properties The names and values of the host properties to change.
 */
bool
S9sRpcClient::setHost(
        const S9sVariantList &hosts,
        const S9sVariantMap  &properties)
{
    S9sString      uri = "/v2/host";
    S9sVariantMap  request;
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("setHost is currently implemented only for one node.");
        return false;
    }

    request["operation"]  = "setHost";
    request["properties"] = properties;
    if (hosts[0].isNode())
    {
        request["hostname"]   = hosts[0].toNode().hostName();

        if (hosts[0].toNode().hasPort())
            request["port"]   = hosts[0].toNode().port();
    } else {
        request["hostname"]   = hosts[0].toString();
    }
    
    return executeRequest(uri, request);
}

/**
 *
{
    "cc_timestamp": 1475217315,
    "data": [ 
    {
        "class_name": "CmonCpuInfo",
        "cpucores": 4,
        "cpumaxmhz": 2.333e+06,
        "cpumhz": 2000,
        "cpumodel": "Intel(R) Xeon(R) CPU           E5345  @ 2.33GHz",
        "cputemp": 53,
        "hostid": 1,
        "physical_cpu_id": 0,
        "siblings": 4,
        "vendor": "GenuineIntel"
    }, 
    {
        "class_name": "CmonCpuInfo",
        "cpucores": 4,
        "cpumaxmhz": 2.333e+06,
        "cpumhz": 2000,
        "cpumodel": "Intel(R) Xeon(R) CPU           E5345  @ 2.33GHz",
        "cputemp": 53,
        "hostid": 4,
        "physical_cpu_id": 1,
        "siblings": 4,
        "vendor": "GenuineIntel"
    } ],
    "requestStatus": "ok",
    "total": 6
}
 */
bool
S9sRpcClient::getCpuInfo(
        const int clusterId)
{
    S9sString      uri = "/v2/stat";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "getCpuPhysicalInfo";
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function will send the "getInfo" call on the "v2/stat" interface. The
 * reply will be something like this:
 * \code{.js}
 * {
 *     "collected_info": [ 
 *     {
 *         "cluster_id": 1,
 *         "info": 
 *         {
 *             "cluster.status": 2,
 *             "cluster.statustext": "Cluster started.",
 *             "cmon.domainname": "",
 *             "cmon.hostname": "t7500",
 *             "cmon.running": true,
 *             "cmon.starttime": 1501144642,
 *             "cmon.uptime": 15636,
 *             "conf.backup_retention": 31,
 *             "conf.clusterid": 1,
 *             "conf.clustername": "ft_postgresql_48154",
 *             "conf.clustertype": 5,
 *             "conf.configfile": "/tmp/cmon_1.cnf",
 *             "conf.hostname": "192.168.1.127",
 *             "conf.os": "debian",
 *             "conf.statustext": "Configuration loaded.",
 *             "host.1.connected": true,
 *             "host.1.cpu_io_wait_percent": 0.353357,
 *             "host.1.cpu_steal_percent": 0,
 *             "host.1.cpu_usage_percent": 15.5331,
 *             "host.1.cpucores": 16,
 *             "host.1.cpuinfo": [ 
 *             {
 *                 "class_name": "CmonCpuInfo",
 *                 "cpucores": 4,
 *                 "cpumaxmhz": 2.268e+06,
 *                 "cpumhz": 1600,
 *                 "cpumodel": "Intel(R) Xeon(R) CPU           L5520  @ 2.27GHz",
 *                 "cputemp": 55.5,
 *                 "hostid": 1,
 *                 "physical_cpu_id": 0,
 *                 "siblings": 8,
 *                 "vendor": "GenuineIntel"
 *             }, 
 *             {
 *                 "class_name": "CmonCpuInfo",
 *                 "cpucores": 4,
 *                 "cpumaxmhz": 2.268e+06,
 *                 "cpumhz": 1733,
 *                 "cpumodel": "Intel(R) Xeon(R) CPU           L5520  @ 2.27GHz",
 *                 "cputemp": 55.5,
 *                 "hostid": 1,
 *                 "physical_cpu_id": 1,
 *                 "siblings": 8,
 *                 "vendor": "GenuineIntel"
 *             } ],
 *             "host.1.cpumaxmhz": 2268,
 *             "host.1.cpumhz": 1733,
 *             "host.1.cpumodel": "Intel(R) Xeon(R) CPU           L5520  @ 2.27GHz",
 *             "host.1.cputemp": 55.5,
 *             "host.1.devices": [ "/dev/mapper/core1--vg-root" ],
 *             "host.1.free_disk_bytes": 155814936576,
 *             "host.1.hostname": "192.168.1.174",
 *             "host.1.interfaces": [ "eth0" ],
 *             "host.1.ip": "192.168.1.174",
 *             "host.1.membuffer": 0,
 *             "host.1.memcached": 1,
 *             "host.1.memfree": 16031900,
 *             "host.1.memtotal": 16777216,
 *             "host.1.network_interfaces": [ 
 *             {
 *                 "interface_name": "eth0",
 *                 "rx_bytes_per_sec": 11775.8,
 *                 "tx_bytes_per_sec": 17652.3
 *             } ],
 *             "host.1.pingdelay": -1,
 *             "host.1.pingstatustext": "Creating ICMP socket (to ping '192.168.1.174') failed: Operation not permitted.",
 *             "host.1.port": 8089,
 *             "host.1.rx_bytes_per_second": 11775.8,
 *             "host.1.swapfree": 0,
 *             "host.1.swaptotal": 0,
 *             "host.1.total_disk_bytes": 208033853440,
 *             "host.1.tx_bytes_per_second": 17652.3,
 *             "host.1.uptime": 15779,
 *             "host.1.wallclock": 1501160299,
 *             "host.1.wallclocksampled": 1501160235,
 *             "host.2.class_name": "controller",
 *             "host.2.connected": true,
 *             "host.2.cpu_io_wait_percent": 2.89087,
 *             "host.2.cpu_steal_percent": 0,
 *             "host.2.cpu_usage_percent": 204.378,
 *             "host.2.cpucores": 24,
 *             "host.2.cpuinfo": [ 
 *             {
 *                 "class_name": "CmonCpuInfo",
 *                 "cpucores": 6,
 *                 "cpumaxmhz": 2.661e+06,
 *                 "cpumhz": 1596,
 *                 "cpumodel": "Intel(R) Xeon(R) CPU           X5650  @ 2.67GHz",
 *                 "cputemp": 0,
 *                 "hostid": 2,
 *                 "physical_cpu_id": 0,
 *                 "siblings": 12,
 *                 "vendor": "GenuineIntel"
 *             }, 
 *             {
 *                 "class_name": "CmonCpuInfo",
 *                 "cpucores": 6,
 *                 "cpumaxmhz": 2.661e+06,
 *                 "cpumhz": 1596,
 *                 "cpumodel": "Intel(R) Xeon(R) CPU           X5650  @ 2.67GHz",
 *                 "cputemp": 0,
 *                 "hostid": 2,
 *                 "physical_cpu_id": 1,
 *                 "siblings": 12,
 *                 "vendor": "GenuineIntel"
 *             } ],
 *             "host.2.cpumaxmhz": 2661,
 *             "host.2.cpumhz": 1596,
 *             "host.2.cpumodel": "Intel(R) Xeon(R) CPU           X5650  @ 2.67GHz",
 *             "host.2.cputemp": 0,
 *             "host.2.devices": [ "/dev/sda1" ],
 *             "host.2.free_disk_bytes": 1409452683264,
 *             "host.2.hostname": "192.168.1.127",
 *             "host.2.interfaces": [ "eth0" ],
 *             "host.2.ip": "192.168.1.127",
 *             "host.2.membuffer": 318624,
 *             "host.2.memcached": 19415480,
 *             "host.2.memfree": 9243460,
 *             "host.2.memtotal": 49453276,
 *             "host.2.network_interfaces": [ 
 *             {
 *                 "interface_name": "eth0",
 *                 "rx_bytes_per_sec": 29150.4,
 *                 "tx_bytes_per_sec": 18520.8
 *             } ],
 *             "host.2.pingdelay": -1,
 *             "host.2.pingstatustext": "Creating ICMP socket (to ping '192.168.1.127') failed: Operation not permitted.",
 *             "host.2.port": 9555,
 *             "host.2.rx_bytes_per_second": 29150.4,
 *             "host.2.swapfree": 0,
 *             "host.2.swaptotal": 0,
 *             "host.2.total_disk_bytes": 2125259440128,
 *             "host.2.tx_bytes_per_second": 18520.8,
 *             "host.2.uptime": 2.59852e+06,
 *             "host.2.version": "1.4.3",
 *             "host.2.wallclock": 1501160235,
 *             "host.2.wallclocksampled": 1501160235,
 *             "license.expires": -1,
 *             "license.status": false,
 *             "license.statustext": "No license found.",
 *             "mail.statustext": "Created.",
 *             "netStat.1.eth0.rxBytes": 287358005,
 *             "netStat.1.eth0.txBytes": 205682361,
 *             "netStat.2.eth0.rxBytes": 531084878595,
 *             "netStat.2.eth0.txBytes": 503628558465
 *         }
 *     } ],
 *     "request_created": "2017-07-27T12:58:00.283Z",
 *     "request_id": 3,
 *     "request_processed": "2017-07-27T12:58:00.328Z",
 *     "request_status": "Ok",
 *     "request_user_id": 3
 * }
 * \endcode
 */
bool
S9sRpcClient::getInfo()
{
    S9sString      uri = "/v2/stat";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getInfo";

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId the ID of the cluster for which the statistics will be
 *   fetched.
 * \returns true if the request sent and a reply is received (even if the reply
 *   is an error message).
 */
bool
S9sRpcClient::getCpuStats(
        const int clusterId)
{
    return getStats(clusterId, "cpustat");
}

/**
 * \param clusterId the ID of the cluster for which the statistics will be
 *   fetched.
 * \returns true if the request sent and a reply is received (even if the reply
 *   is an error message).
 */
bool
S9sRpcClient::getSqlStats(
        const int clusterId)
{
    return getStats(clusterId, "sqlstat");
}

/**
 * \param clusterId the ID of the cluster for which the statistics will be
 *   fetched.
 * \returns true if the request sent and a reply is received (even if the reply
 *   is an error message).
 */
bool
S9sRpcClient::getMemStats(
        const int clusterId)
{
    return getStats(clusterId, "memorystat");
}

bool
S9sRpcClient::getMetaTypes()
{
    S9sString      uri = "/v2/metatype/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getMetaTypes";

    retval = executeRequest(uri, request);
    
    return retval;    
}

bool
S9sRpcClient::getMetaTypeProperties(
        const S9sString &typeName)
{
    S9sString      uri = "/v2/metatype/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getMetaTypeInfo";
    request["type-name"] = typeName;

    retval = executeRequest(uri, request);
    
    return retval;    
}

/**
 * The reply will contain a lot of these:
    {
        "created": 1493297858,
        "hostid": 2,
        "interval": 67335,
        "memoryutilization": 0.0959179,
        "pgpgin": 0,
        "pgpgout": 3124,
        "pswpin": 0,
        "pswpout": 0,
        "rambuffers": 0,
        "ramcached": 7168,
        "ramfree": 15532005376,
        "ramfreemin": 15531171840,
        "ramtotal": 17179869184,
        "sampleends": 1493297918,
        "samplekey": "CmonMemoryStats-2",
        "swapfree": 0,
        "swaptotal": 0,
        "swaputilization": 0
    }, 
 */
bool
S9sRpcClient::getMemoryStats(
        const int clusterId)
{
    S9sString      uri = "/v2/stat";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "statByName";
    request["name"]       = "memorystat";
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);
    
    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A method to get the list of the running processes from all nodes of one
 * particular cluster.
 */
bool 
S9sRpcClient::getRunningProcesses()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/process";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "getRunningProcesses";
    
    if (options->hasClusterIdOption())
        request["cluster_id"] = options->clusterId();

    if (options->hasClusterNameOption())
        request["cluster_name"] = options->clusterName();


    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId the ID of the cluster for which the job instances will be
 *   fetched.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 * 
 * Sends a "getJobInstances" request, receives the reply. We use this RPC call
 * to get the job list (e.g. s9s job --list).
 */
bool
S9sRpcClient::getJobInstances(
        const S9sString  &clusterName, 
        const int         clusterId)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getJobInstances";

    if (options->limit() >= 0)
        request["limit"] = options->limit();

    if (options->offset() >= 0)
        request["offset"] = options->offset();

    if (S9S_CLUSTER_ID_IS_VALID(clusterId))
        request["cluster_id"] = clusterId;
    
    if (!clusterName.empty())
        request["cluster_name"] = clusterName;

    retval = executeRequest(uri, request);

    return retval;
}



/**
 * \param clusterId the ID of the cluster
 * \param jobId the ID of the job
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 * This function sends a "getJobInstance" request to the controller and receives
 * its reply. This request can be used to get the properties of one particular
 * job.
 */
bool
S9sRpcClient::getJobInstance(
        const int jobId)
{
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getJobInstance";
    request["job_id"]    = jobId;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::deleteJobInstance(
        const int jobId)
{
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "deleteJobInstance";
    request["job_id"]    = jobId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param jobId the ID of the job
 * \param limit the maximum number of log entries we are ready to process
 * \param offset the number of log entries to skip
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 * This function will get the log entries in ascending order. This is because
 * the terminal normally used like that.
 */
bool
S9sRpcClient::getJobLog(
        const int jobId,
        const int limit,
        const int offset)
{
    S9sString      uri = "/v2/jobs/";
    S9sVariantMap  request;
    bool           retval;

    // Building the request.
    request["operation"]  = "getJobLog";
    request["job_id"]     = jobId;
    request["ascending"]  = true;
    
    if (limit != 0)
        request["limit"]  = limit;

    if (offset != 0)
        request["offset"] = offset;

    retval = executeRequest(uri, request);

    return retval;

}

/**
 * This function gets the logs from the controller. Not the job messages, but
 * the actual cmon logs.
 *
 * Here is an example of a reply. We might have to double check the format
 * string for this, there are a few fields just added.
 *
 * \code{.js}
 * {
 *     "log_entries": [ 
 *     {
 *         "class_name": "CmonLogMessage",
 *         "component": "ClusterConfiguration",
 *         "created": "2017-07-26T08:35:40.704Z",
 *         "log_class": "LogMessage",
 *         "log_id": 110,
 *         "log_origins": 
 *         {
 *             "sender_binary": "cmon",
 *             "sender_file": "../../src/cmonhostmanager.cpp",
 *             "sender_line": 594,
 *             "sender_pid": 40818,
 *             "tv_nsec": 704637455,
 *             "tv_sec": 1501058140
 *         },
 *         "log_specifics": 
 *         {
 *             "cluster_id": 1,
 *             "message_text": "Registering CmonPostgreSqlHost: 192.168.1.167:8089"
 *         },
 *         "severity": "LOG_DEBUG"
 *     }, 
 *  
 *     . . .
 * 
 *     {
 *         "class_name": "CmonLogMessage",
 *         "created": "2017-07-26T08:38:11.697Z",
 *         "log_class": "LogMessage",
 *         "log_id": 351,
 *         "log_origins": 
 *         {
 *             "sender_binary": "cmon",
 *             "sender_file": "../../src/cmonalarmdb.cpp",
 *             "sender_line": 1467,
 *             "sender_pid": 40818,
 *             "tv_nsec": 697060177,
 *             "tv_sec": 1501058291
 *         },
 *         "log_specifics": 
 *         {
 *             "cluster_id": 1,
 *             "message_text": "hostId: 1, nodeId: 0, alarm: Host is not responding"
 *         },
 *         "severity": "LOG_DEBUG"
 *     } ],
 *     "log_entry_counts": 
 *     {
 *         "component": 
 *         {
 *             "Cluster": 1,
 *             "ClusterConfiguration": 2,
 *             "Node": 3,
 *             "Unknown": 194
 *         },
 *         "hostname": 
 *         {
 *             "": 199,
 *             "192.168.1.167": 1
 *         },
 *         "sender_binary": 
 *         {
 *             "cmon": 200
 *         },
 *         "severity": 
 *         {
 *             "LOG_CRIT": 2,
 *             "LOG_DEBUG": 192,
 *             "LOG_ERR": 1,
 *             "LOG_INFO": 2,
 *             "LOG_WARNING": 3
 *         }
 *     },
 *     "reply_received": "2017-07-26T08:39:53.936Z",
 *     "request_created": "2017-07-26T08:39:53.931Z",
 *     "request_id": 3,
 *     "request_processed": "2017-07-26T08:39:54.019Z",
 *     "request_status": "Ok",
 *     "request_user_id": 3,
 *     "total": 258
 * }
 * \endcode
 */
bool
S9sRpcClient::getLog()
{
    #if 0
    S9sRpcReply theReply;

    generateReport();
    theReply = reply();
    theReply.printReport();

    deleteReport(1);
    //getReports();
    #endif

    S9sOptions    *options   = S9sOptions::instance();
    int            limit     = options->limit();
    int            offset    = options->offset();
    S9sString      uri = "/v2/log/";
    S9sVariantMap  request;
    bool           retval;

    // Building the request.
    request["operation"]  = "getLogEntries";
    request["ascending"]  = true;
    
    if (!options->from().empty())
        request["created_after"] = options->from();

    if (!options->until().empty())
        request["created_before"] = options->until();

        
    if (limit > 0)
        request["limit"]  = limit;

    if (offset > 0)
        request["offset"] = offset;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * Gets the statistics about the log. Here is an example reply:
 *
 * \code{.js}
 * {
 *     "log_statistics": 
 *     {
 *         "cluster_log_statistics": [ 
 *         {
 *             "cluster_id": -1,
 *             "disabled": false,
 *             "entries_received": 3,
 *             "format_string": "%C : (%S) %M",
 *             "last_error_message": "",
 *             "lines_written": 0,
 *             "log_file_name": "",
 *             "max_log_file_size": 5242880,
 *             "messages_per_sec": 0.05,
 *             "syslog_enabled": false,
 *             "write_cycle_counter": 1
 *         }, 
 *         {
 *             "cluster_id": 0,
 *             "disabled": false,
 *             "entries_received": 93,
 *             "format_string": "%C : (%S) %M",
 *             "last_error_message": "Success.",
 *             "lines_written": 93,
 *             "log_file_name": "./cmon-ft-install.log",
 *             "max_log_file_size": 5242880,
 *             "messages_per_sec": 0.05,
 *             "syslog_enabled": false,
 *             "write_cycle_counter": 3
 *         }, 
 *         {
 *             "cluster_id": 1,
 *             "disabled": false,
 *             "entries_received": 218,
 *             "format_string": "%C : (%S) %M",
 *             "last_error_message": "Success.",
 *             "lines_written": 218,
 *             "log_file_name": "/tmp/cmon_1.log",
 *             "max_log_file_size": 5242880,
 *             "messages_per_sec": 0.85,
 *             "syslog_enabled": false,
 *             "write_cycle_counter": 11
 *         } ],
 *         "current_time": "2017-07-26T11:01:08.884Z",
 *         "entries_statistics": 
 *         {
 *             "entries_received": 319,
 *             "entries_written_to_cmondb": 314
 *         },
 *         "has_cmondb": true,
 *         "last_error_message": "Success.",
 *         "last_flush_time": "2017-07-26T11:00:59.543Z",
 *         "log_debug_enabled": false,
 *         "writer_thread_running": true,
 *         "writer_thread_started": "2017-07-26T10:55:29.410Z"
 *     },
 *     "request_created": "2017-07-26T11:01:08.840Z",
 *     "request_id": 3,
 *     "request_processed": "2017-07-26T11:01:08.884Z",
 *     "request_status": "Ok",
 *     "request_user_id": 3
 * }
 * \endcode
 */
bool
S9sRpcClient::getLogStatistics()
{
    S9sString      uri = "/v2/log/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getLogStatistics";
    return executeRequest(uri, request);
}


/**
 * Gets the active alarms of a cluster. Here is an example that shows the reply:
 * \code{.js}
 * {
 *     "alarms": [ 
 *     {
 *         "alarm_id": 1,
 *         "class_name": "CmonAlarm",
 *         "cluster_id": 1,
 *         "component": 3,
 *         "component_name": "Cluster",
 *         "counter": 1,
 *         "created": "2017-07-27T06:33:38.000Z",
 *         "ignored": 0,
 *         "measured": 0,
 *         "message": "System time is drifting between servers and the distance between the highest system time and lowest is more than one minute.",
 *         "recommendation": "Synchronize the system clock on the servers using e.g NTP or make sure NTP is working. Time drifting may lead to unexpected failures or problems, and makes debugging hard.\n\nThe last seen host time values (in controller's time-zone):\n192.168.1.127: Jul 27 08:32:40\n192.168.1.169: Jul 27 08:33:44\n",
 *         "reported": "2017-07-27T06:33:38.000Z",
 *         "severity": 1,
 *         "severity_name": "ALARM_WARNING",
 *         "title": "System time is drifting",
 *         "type": 40000,
 *         "type_name": "ClusterTimeDrift"
 *     }, 
 *     . . .
 *     {
 *         "alarm_id": 3,
 *         "class_name": "CmonAlarm",
 *         "cluster_id": 1,
 *         "component": 0,
 *         "component_name": "Network",
 *         "counter": 12,
 *         "created": "2017-07-27T06:35:56.000Z",
 *         "host_id": 1,
 *         "hostname": "192.168.1.169",
 *         "ignored": 0,
 *         "measured": 0,
 *         "message": "Server 192.168.1.169 reports: Host 192.168.1.169 is not responding to ping after 3 cycles, the host is most likely unreachable.",
 *         "recommendation": "Restart failed host, check firewall.",
 *         "reported": "2017-07-27T06:35:56.000Z",
 *         "severity": 2,
 *         "severity_name": "ALARM_CRITICAL",
 *         "title": "Host is not responding",
 *         "type": 10006,
 *         "type_name": "HostUnreachable"
 *     } ],
 *     "cluster_id": 1,
 *     "request_created": "2017-07-27T06:36:03.627Z",
 *     "request_id": 3,
 *     "request_processed": "2017-07-27T06:36:03.675Z",
 *     "request_status": "Ok",
 *     "request_user_id": 3
 * }
 * \endcode
 */
bool
S9sRpcClient::getAlarms()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/alarm/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getAlarms";
    
    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getAlarm()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/alarm/";
    S9sVariantMap  request;
    int            alarmId = 1;

    // Building the request.
    request["operation"]  = "getAlarm";
    request["alarm_id"]   = alarmId;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getAlarmStatistics()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/alarm/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getStatistics";

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getReports()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/reports/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getReports";

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::generateReport()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/reports/";
    S9sVariantMap  request;
    S9sVariantMap  reportMap;

    // Building the request.
    reportMap["class_name"]  = "CmonReport";
    reportMap["report_type"] = "testreport";
    reportMap["recipients"]  = "laszlo@severalnines.com";
    reportMap["text_format"] = "AnsiTerminal";

    request["operation"]     = "generateReport";
    request["report"]        = reportMap;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::deleteReport(
        const int reportId)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/reports/";
    S9sVariantMap  request;
    S9sVariantMap  reportMap;

    // Building the request.
    reportMap["class_name"]  = "CmonReport";
    reportMap["report_id"]   = reportId;

    request["operation"]     = "deleteReport";
    request["report"]        = reportMap;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

/**
 * This will initiate a job that creates a local repository.
 */
bool
S9sRpcClient::createLocalRepository(
        const int          clusterId,
        const S9sString   &clusterType,
        const S9sString   &vendor,
        const S9sString   &dbVersion,
        const S9sString   &osRelease)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";

    jobData["cluster_type"] = clusterType;
    jobData["vendor"]     = vendor;
    jobData["db_version"] = dbVersion;
    jobData["os_release"] = osRelease;
    //jobData["dry_run"]    = true;

    jobSpec["command"]    = "create_local_repository";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Create Repository";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule(); 

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;
    
    return executeRequest(uri, request);
}

/**
 * \param clusterId the ID of the cluster that will be restarted
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 * Creates a job for "rolling restart" and receives the controller's answer for
 * the request. 
 */
bool
S9sRpcClient::rollingRestart(
        const int clusterId)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantMap  request;
    S9sVariantMap  job, jobSpec;
    S9sString      uri = "/v2/jobs/";

    jobSpec["command"]    = "rolling_restart";

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Rolling Restart";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule(); 

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;
    
    return executeRequest(uri, request);
}

/**
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 */
bool
S9sRpcClient::createReport(
        const int clusterId)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      outputDir = options->outputDir();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;
    
    if (!outputDir.empty())
        jobData["report_dir"] = outputDir;

    jobSpec["command"]    = "error_report";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Create Error Report";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule(); 

    // The request describing we want to register a job instance.    
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;
    
    retval = executeRequest(uri, request);

    return retval;
}


/**
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply), false if the request was
 *   not even sent.
 *
 */
bool
S9sRpcClient::createCluster()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantList hosts;
    S9sString      osUserName;
    S9sString      vendor;
    S9sString      dbVersion;
    bool           uninstall = true;
    S9sRpcReply    reply;
    bool           success;

    if (options->clusterType().empty())
    {
        PRINT_ERROR(
                 "Cluster type is not set.\n"
                 "Use the --cluster-type command line option to set it.");

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    hosts = options->nodes();
    if (hosts.empty())
    {
        PRINT_ERROR(
            "Node list is empty while creating cluster.\n"
            "Use the --nodes command line option to provide the node list."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    dbVersion    = options->providerVersion("5.6");
    osUserName   = options->osUser();
    vendor       = options->vendor();

    if (vendor.empty() && options->clusterType() != "postgresql")
    {
        PRINT_ERROR(
            "The vendor name is unknown while creating a cluster.\n"
            "Use the --vendor command line option to provide the vendor."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    if (dbVersion.empty())
    {
        PRINT_ERROR(
            "The SQL server version is unknown while creating a cluster.\n"
            "Use the --provider-version command line option set it."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    /*
     * Running the request on the controller.
     */
    if (options->clusterType() == "mysql_single" ||
            options->clusterType() == "mysql-single")
    {
        success = createMySqlSingleCluster(
                hosts, osUserName, vendor, dbVersion, uninstall);
    } else if (options->clusterType() == "galera")
    {
        success = createGaleraCluster(
                hosts, osUserName, vendor, dbVersion, uninstall);
    } else if (options->clusterType() == "mysqlreplication")
    {
        success = createMySqlReplication(
                hosts, osUserName, vendor, dbVersion, uninstall);
    } else if (options->clusterType() == "group_replication" || 
            options->clusterType() == "groupreplication")
    {
        success = createGroupReplication(
                hosts, osUserName, vendor, dbVersion, uninstall);
    } else if (options->clusterType() == "postgresql")
    {
        success = createPostgreSql(hosts, osUserName, uninstall);
    } else if (options->clusterType() == "ndb" || 
            options->clusterType() == "ndbcluster")
    {
        S9sVariantList mySqlHosts, mgmdHosts, ndbdHosts;

        for (uint idx = 0u; idx < hosts.size(); ++idx)
        {
            S9sNode     node     = hosts[idx].toNode();
            S9sString   protocol = node.protocol().toLower();

            if (protocol == "ndbd")
            {
                ndbdHosts << node;
            } else if (protocol == "mgmd" || protocol == "ndb_mgmd")
            {
                mgmdHosts << node;
            } else if (protocol == "mysql" || protocol.empty())
            {
                mySqlHosts << node;
            } else {
                PRINT_ERROR(
                        "The protocol '%s' is not supported.", 
                        STR(protocol));
                return false;
            }
        }

        success = createNdbCluster(
                mySqlHosts, mgmdHosts, ndbdHosts,
                osUserName, vendor, dbVersion, uninstall);
    } else {
        success = false;
    }

    return success;
}

bool
S9sRpcClient::registerCluster()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantList hosts;
    S9sString      osUserName;
    bool           success = false;

    hosts = options->nodes();
    if (hosts.empty())
    {
        PRINT_ERROR(
            "Node list is empty while registering cluster.\n"
            "Use the --nodes command line option to provide the node list."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return success;
    }
    
    osUserName   = options->osUser();
    
    if (options->clusterType() == "postgresql")
    {
        success = registerPostgreSql(hosts, osUserName);
    } else if (options->clusterType() == "galera")
    {
        success = registerGaleraCluster(hosts, osUserName);
    } else if (options->clusterType() == "mysqlreplication")
    {
        success = registerMySqlReplication(hosts, osUserName);
    } else if (options->clusterType() == "group_replication")
    {
        success = registerGroupReplication(hosts, osUserName);
    } else if (options->clusterType() == "ndb" || 
            options->clusterType() == "ndbcluster")
    {
        S9sVariantList mySqlHosts, mgmdHosts, ndbdHosts;

        for (uint idx = 0u; idx < hosts.size(); ++idx)
        {
            S9sNode     node     = hosts[idx].toNode();
            S9sString   protocol = node.protocol().toLower();

            if (protocol == "ndbd")
            {
                ndbdHosts << node;
            } else if (protocol == "mgmd" || protocol == "ndb_mgmd")
            {
                mgmdHosts << node;
            } else if (protocol == "mysql" || protocol.empty())
            {
                mySqlHosts << node;
            } else {
                PRINT_ERROR(
                        "The protocol '%s' is not supported.", 
                        STR(protocol));
                return false;
            }
        }

        success = registerNdbCluster(
                mySqlHosts, mgmdHosts, ndbdHosts, osUserName);
    } else {
        PRINT_ERROR("Register cluster is currently implemented only for "
                "some cluster types.");
        options->setExitStatus(S9sOptions::BadOptions);
        return success;
    }

    return success;
}

/**
 * \param clusterId the ID of the cluster for which the CPU information will be
 *   fetched.
 * \param statName cpustat sqlstatsum sqlstat
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::getStats(
        const int        clusterId,
        const S9sString &statName)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      begin   = options->begin();
    S9sString      end     = options->end();
    S9sString      uri = "/v2/stat";
    S9sVariantMap  request;
    bool           retval;
    time_t         now = time(NULL);

    request["operation"]  = "statByName";
    request["name"]       = statName;
    request["with_hosts"] = true;
    request["cluster_id"] = clusterId;

    if (!begin.empty())
        request["start_datetime"] = begin;

    if (!end.empty())
        request["end_datetime"] = end;

    if (begin.empty() && end.empty())
    {
        request["startdate"]  = (ulonglong) now - 60 * 60;
        request["enddate"]    = (ulonglong) now;
    }

    retval = executeRequest(uri, request);
    
    return retval;
}


/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \param osUserName the user name to be used to SSH to the host.
 * \param vendor the name of the database vendor to install.
 * \param mySqlVersion the MySql version to install.
 * \param uninstall true if uninstalling the existing software is allowed.
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 *
 */
bool
S9sRpcClient::createGaleraCluster(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        const S9sString      &vendor,
        const S9sString      &mySqlVersion,
        bool                  uninstall)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";
    
    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Galera cluster.");
        return false;
    }
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "galera";
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = vendor;
    jobData["mysql_version"]    = mySqlVersion;
    jobData["enable_mysql_uninstall"] = uninstall;
    jobData["ssh_user"]         = osUserName;
    jobData["mysql_password"]   = options->dbAdminPassword();
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();
    
    if (!options->osKeyFile().empty())
        jobData["ssh_key"]      = options->osKeyFile();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "create_cluster";
    jobSpec["job_data"]         = jobData;

    // 
    // The job instance describing how the job will be executed.
    //
    job["class_name"]           = "CmonJobInstance";
    job["title"]                = "Create Galera Cluster";
    job["job_spec"]             = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]        = options->schedule(); 

    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::createMySqlSingleCluster(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        const S9sString      &vendor,
        const S9sString      &mySqlVersion,
        bool                  uninstall)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";
    
    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Galera cluster.");
        return false;
    }
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "mysql_single";
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = vendor;
    jobData["mysql_version"]    = mySqlVersion;
    jobData["enable_mysql_uninstall"] = uninstall;
    jobData["ssh_user"]         = osUserName;
    jobData["mysql_password"]   = options->dbAdminPassword();
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();
    
    if (!options->osKeyFile().empty())
        jobData["ssh_key"]      = options->osKeyFile();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "create_cluster";
    jobSpec["job_data"]         = jobData;

    // 
    // The job instance describing how the job will be executed.
    //
    job["class_name"]           = "CmonJobInstance";
    job["title"]                = "Create Single MySql Instance";
    job["job_spec"]             = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]        = options->schedule(); 

    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * http://52.58.107.236/cmon-docs/current/cmonjobs.html#add_cluster1
 */
bool
S9sRpcClient::registerGaleraCluster(
        const S9sVariantList &hosts,
        const S9sString      &osUserName)

{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";

    if (hosts.size() < 1u)
    {
        PRINT_ERROR(
                "Nodes are not specified while registering existing cluster.");
        return false;
    }
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "galera";
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = options->vendor();
    jobData["ssh_user"]         = osUserName;
    
    if (!options->osKeyFile().empty())
        jobData["ssh_keyfile"]  = options->osKeyFile();

    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "add_cluster";
    jobSpec["job_data"]         = jobData;
    
    // 
    // The job instance describing how the job will be executed.
    //
    job["class_name"]           = "CmonJobInstance";
    job["title"]                = "Register Galera";
    job["job_spec"]             = jobSpec;
    
    if (!options->schedule().empty())
        job["scheduled"]        = options->schedule(); 
    
    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \param osUserName the user name to be used to SSH to the host.
 * \param vendor the name of the database vendor to install.
 * \param mySqlVersion the MySql version to install. 
 * \param uninstall true if uninstalling the existing software is allowed.
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 */
bool
S9sRpcClient::createMySqlReplication(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        const S9sString      &vendor,
        const S9sString      &mySqlVersion,
        bool                  uninstall)
{
    S9sOptions     *options = S9sOptions::instance();
    //S9sVariantList  hostNames;
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";
    bool            retval;
    
    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Galera cluster.");
        return false;
    }
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "replication";
    jobData["topology"]         = topologyField(hosts);
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = vendor;
    jobData["mysql_version"]    = mySqlVersion;
    jobData["enable_mysql_uninstall"] = uninstall;
    jobData["type"]             = "mysql";
    jobData["ssh_user"]         = osUserName;
    jobData["repl_user"]        = options->dbAdminUserName();
    jobData["repl_password"]    = options->dbAdminPassword();
   
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    if (!options->osKeyFile().empty())
        jobData["ssh_key"]      = options->osKeyFile();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]    = "create_cluster";
    jobSpec["job_data"]   = jobData;

    // 
    // The job instance describing how the job will be executed.
    //
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Create MySQL Replication Cluster";
    job["job_spec"]       = jobSpec;

    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = 0;
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * http://52.58.107.236/cmon-docs/current/cmonjobs.html#add_cluster1
 */
bool
S9sRpcClient::registerMySqlReplication(
        const S9sVariantList &hosts,
        const S9sString      &osUserName)

{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";

    if (hosts.size() < 1u)
    {
        PRINT_ERROR(
                "Nodes are not specified while registering existing cluster.");
        return false;
    }
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "replication";
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = options->vendor();
    jobData["ssh_user"]         = osUserName;
    
    if (!options->osKeyFile().empty())
        jobData["ssh_keyfile"]  = options->osKeyFile();

    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "add_cluster";
    jobSpec["job_data"]         = jobData;
    
    // 
    // The job instance describing how the job will be executed.
    //
    job["class_name"]           = "CmonJobInstance";
    job["title"]                = "Register MySql Replication";
    job["job_spec"]             = jobSpec;
    
    if (!options->schedule().empty())
        job["scheduled"]        = options->schedule(); 
    
    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \param osUserName the user name to be used to SSH to the host.
 * \param vendor the name of the database vendor to install.
 * \param mySqlVersion the MySql version to install. 
 * \param uninstall true if uninstalling the existing software is allowed.
 * \returns true if the operation was successful, a reply is received from the
 *   controller (even if the reply is an error reply).
 */
bool
S9sRpcClient::createGroupReplication(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        const S9sString      &vendor,
        const S9sString      &mySqlVersion,
        bool                  uninstall)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  hostNames;
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";
    bool            retval;
    
    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Galera cluster.");
        return false;
    }
    
    for (uint idx = 0; idx < hosts.size(); ++idx)
    {
        if (hosts[idx].isNode())
            hostNames << hosts[idx].toNode().hostName();
        else
            hostNames << hosts[idx];
    }

    // The job_data describing the cluster.
    jobData["cluster_type"]     = "group_replication";
    jobData["mysql_hostnames"]  = hostNames;
    jobData["master_address"]   = hostNames[0].toString();
    jobData["vendor"]           = vendor;
    jobData["mysql_version"]    = mySqlVersion;
    jobData["enable_mysql_uninstall"] = uninstall;
    jobData["type"]             = "mysql";
    jobData["ssh_user"]         = osUserName;
    jobData["repl_user"]        = options->dbAdminUserName();
    jobData["repl_password"]    = options->dbAdminPassword();
   
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    if (!options->osKeyFile().empty())
        jobData["ssh_key"]      = options->osKeyFile();

    // The jobspec describing the command.
    jobSpec["command"]    = "create_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Create MySQL Replication Cluster";
    job["job_spec"]       = jobSpec;

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = 0;
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * http://52.58.107.236/cmon-docs/current/cmonjobs.html#add_cluster1
 */
bool
S9sRpcClient::registerGroupReplication(
        const S9sVariantList &hosts,
        const S9sString      &osUserName)

{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";

    if (hosts.size() < 1u)
    {
        PRINT_ERROR(
                "Nodes are not specified while registering existing cluster.");
        return false;
    }
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "group_replication";
    jobData["nodes"]            = nodesField(hosts);
    jobData["vendor"]           = options->vendor();
    jobData["ssh_user"]         = osUserName;
    
    if (!options->osKeyFile().empty())
        jobData["ssh_keyfile"]  = options->osKeyFile();

    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "add_cluster";
    jobSpec["job_data"]         = jobData;
    
    // 
    // The job instance describing how the job will be executed.
    //
    job["class_name"]           = "CmonJobInstance";
    job["title"]                = "Register MySql Replication";
    job["job_spec"]             = jobSpec;
    
    if (!options->schedule().empty())
        job["scheduled"]        = options->schedule(); 
    
    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 *
 */
bool
S9sRpcClient::createNdbCluster(
        const S9sVariantList &mySqlHosts,
        const S9sVariantList &mgmdHosts,
        const S9sVariantList &ndbdHosts,
        const S9sString      &osUserName, 
        const S9sString      &vendor,
        const S9sString      &mySqlVersion,
        bool                  uninstall)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  mySqlHostNames, mgmdHostNames, ndbdHostNames;
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";
    bool            retval;
    
    for (uint idx = 0; idx < mySqlHosts.size(); ++idx)
    {
        if (mySqlHosts[idx].isNode())
            mySqlHostNames << mySqlHosts[idx].toNode().hostName();
        else
            mySqlHostNames << mySqlHosts[idx];
    }
    
    for (uint idx = 0; idx < mgmdHosts.size(); ++idx)
    {
        if (mgmdHosts[idx].isNode())
            mgmdHostNames << mgmdHosts[idx].toNode().hostName();
        else
            mgmdHostNames << mgmdHosts[idx];
    }
    
    for (uint idx = 0; idx < ndbdHosts.size(); ++idx)
    {
        if (ndbdHosts[idx].isNode())
            ndbdHostNames << ndbdHosts[idx].toNode().hostName();
        else
            ndbdHostNames << ndbdHosts[idx];
    }
    
    // The job_data describing the cluster.
    jobData["cluster_type"]     = "mysqlcluster";
    jobData["type"]             = "mysql";
    jobData["mysql_hostnames"]  = mySqlHostNames;
    jobData["mgmd_hostnames"]   = mgmdHostNames;
    jobData["ndbd_hostnames"]   = ndbdHostNames;
    jobData["ssh_user"]         = osUserName;
    jobData["vendor"]           = vendor;
    jobData["mysql_version"]    = mySqlVersion;
    jobData["disable_selinux"]  = true;
    jobData["disable_firewall"] = true;
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    if (!options->osKeyFile().empty())
        jobData["ssh_key"] = options->osKeyFile();

    // The jobspec describing the command.
    jobSpec["command"]    = "create_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Create NDB Cluster";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"] = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = 0;
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 *
 */
bool
S9sRpcClient::registerNdbCluster(
        const S9sVariantList &mySqlHosts,
        const S9sVariantList &mgmdHosts,
        const S9sVariantList &ndbdHosts,
        const S9sString      &osUserName)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  mySqlHostNames, mgmdHostNames, ndbdHostNames;
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";
    
    for (uint idx = 0; idx < mySqlHosts.size(); ++idx)
    {
        if (mySqlHosts[idx].isNode())
            mySqlHostNames << mySqlHosts[idx].toNode().hostName();
        else
            mySqlHostNames << mySqlHosts[idx];
    }
    
    for (uint idx = 0; idx < mgmdHosts.size(); ++idx)
    {
        if (mgmdHosts[idx].isNode())
            mgmdHostNames << mgmdHosts[idx].toNode().hostName();
        else
            mgmdHostNames << mgmdHosts[idx];
    }
    
    for (uint idx = 0; idx < ndbdHosts.size(); ++idx)
    {
        if (ndbdHosts[idx].isNode())
            ndbdHostNames << ndbdHosts[idx].toNode().hostName();
        else
            ndbdHostNames << ndbdHosts[idx];
    }
    
    // The job_data describing the cluster.
    jobData["cluster_type"]     = "mysqlcluster";
    jobData["type"]             = "mysql";
    jobData["mysql_hostnames"]  = mySqlHostNames;
    jobData["mgmd_hostnames"]   = mgmdHostNames;
    jobData["ndbd_hostnames"]   = ndbdHostNames;
    jobData["ssh_user"]         = osUserName;
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    if (!options->osKeyFile().empty())
        jobData["ssh_key"] = options->osKeyFile();

    // The jobspec describing the command.
    jobSpec["command"]    = "add_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Register NDB Cluster";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"] = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = 0;
    
    return executeRequest(uri, request);
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \param osUserName the user name to be used to SSH to the host.
 * \param uninstall true if uninstalling the existing software is allowed.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * This method will create a job that creates a single server PostgreSQL
 * cluster.
 */
bool
S9sRpcClient::createPostgreSql(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        bool                  uninstall)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";

    if (hosts.size() != 1u)
    {
        PRINT_ERROR(
                "Creating a PostgreSQL cluster currently only possible "
                "with one server in it.");
        return false;
    }

    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "postgresql_single";
    jobData["type"]             = "postgresql";
    
    jobData["hostname"]         = hosts[0].toNode().hostName();

    if (hosts[0].toNode().hasPort())
        jobData["port"] = hosts[0].toNode().port();

    jobData["enable_uninstall"] = uninstall;
    jobData["ssh_user"]         = osUserName;
    jobData["postgre_user"]     = options->dbAdminUserName();
    jobData["postgre_password"] = options->dbAdminPassword();

    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();
    
    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "setup_server";
    jobSpec["job_data"]         = jobData;

    // 
    // The job instance describing how the job will be executed.
    //
    job["class_name"]           = "CmonJobInstance";
    job["title"]                = "Setup PostgreSQL Server";
    job["job_spec"]             = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]        = options->schedule(); 

    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

/**
 * http://52.58.107.236/cmon-docs/current/cmonjobs.html#add_cluster1
 */
bool
S9sRpcClient::registerPostgreSql(
        const S9sVariantList &hosts,
        const S9sString      &osUserName)

{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";

    if (hosts.size() < 1u)
    {
        PRINT_ERROR(
                "Nodes are not specified while registering existing cluster.");
        return false;
    }
    
    // 
    // The job_data describing the cluster.
    //
    jobData["cluster_type"]     = "postgresql_single";
    jobData["nodes"]            = nodesField(hosts);
    jobData["ssh_user"]         = osUserName;
    
    if (!options->osKeyFile().empty())
        jobData["ssh_keyfile"]  = options->osKeyFile();

    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();

    // 
    // The jobspec describing the command.
    //
    jobSpec["command"]          = "add_cluster";
    jobSpec["job_data"]         = jobData;
    
    // 
    // The job instance describing how the job will be executed.
    //
    job["class_name"]           = "CmonJobInstance";
    job["title"]                = "Register PostgreSQL";
    job["job_spec"]             = jobSpec;
    
    if (!options->schedule().empty())
        job["scheduled"]        = options->schedule(); 
    
    // 
    // The request describing we want to register a job instance.
    //
    request["operation"]        = "createJobInstance";
    request["job"]              = job;
    
    return executeRequest(uri, request);
}

bool 
S9sRpcClient::createNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts;
    S9sRpcReply    reply;
    bool           hasHaproxy  = false;
    bool           hasProxySql = false;
    bool           hasMaxScale = false;
    bool           success;

    hosts = options->nodes();
    if (hosts.empty())
    {
        PRINT_ERROR(
                "Node list is empty while adding node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    for (uint idx = 0u; idx < hosts.size(); ++idx)
    {
        S9sString protocol = hosts[idx].toNode().protocol().toLower();

        if (protocol == "haproxy")
            hasHaproxy = true;
        else if (protocol == "proxysql")
            hasProxySql = true;
        else if (protocol == "maxscale")
            hasMaxScale = true;
    }

    /*
     * Running the request on the controller.
     */
    if (hasHaproxy && hasProxySql) 
    {
        PRINT_ERROR(
                "It is not possible to add a HaProxy and a ProxySql node "
                "in one call.");

        return false;
    } else if (hasHaproxy && hasMaxScale)
    {
        PRINT_ERROR(
                "It is not possible to add a HaProxy and a MaxScale node "
                "in one call.");

        return false;
    } else if (hasProxySql && hasMaxScale)
    {
        PRINT_ERROR(
                "It is not possible to add a ProxySql and a MaxScale node "
                "in one call.");

        return false;
    } else if (hasProxySql)
    {
        success = addProxySql(clusterId, hosts);
    } else if (hasHaproxy)
    {
        success = addHaProxy(clusterId, hosts);
    } else if (hasMaxScale)
    {
        success = addMaxScale(clusterId, hosts);
    } else {
        int nSlaves  = 0;
        int nMasters = 0;

        for (uint idx = 0; idx < hosts.size(); ++idx)
        {
            const S9sNode &node = hosts[idx].toNode();
            bool           master = node.property("master").toBoolean();
            bool           slave = node.property("slave").toBoolean();

            S9S_DEBUG("[%02u] %5s %s", 
                    idx, 
                    master ? "true" : "false",
                    slave  ? "true" : "false",
                    STR(node.hostName()));

            if (slave)
                nSlaves += 1;

            if (master)
                nMasters += 1;
        }
        
        if (nSlaves == 0 && nMasters == 0)
            success = addNode(clusterId, hosts);
        else
            success = addReplicationSlave(clusterId, hosts);
    }

    return success;
}



/**
 * \param clusterId The ID of the cluster.
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Creates a job that will add a new node to the cluster.
 */
bool
S9sRpcClient::addNode(
        const int             clusterId,
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;

    if (hosts.size() != 1u)
    {
        PRINT_ERROR("addnode is currently implemented only for one node.");
        return false;
    }
    
    // The job_data describing the cluster.
    if (hosts[0].isNode())
        jobData["hostname"] = hosts[0].toNode().hostName();
    else 
        jobData["hostname"] = hosts[0].toString();

    jobData["install_software"] = true;
    jobData["disable_firewall"] = true;
    jobData["disable_selinux"]  = true;
   
    // The jobspec describing the command.
    jobSpec["command"]    = "addnode";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Add Node to Cluster";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"] = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = clusterId;
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    } else {
        PRINT_ERROR(
                "Either the --cluster-id or the --cluster-name command line "
                "option has to be provided.");
        return false;
    }
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * This method is very similar to the addNode() method, but it adds a slave to
 * an existing cluster.
 */
bool
S9sRpcClient::addReplicationSlave(
        const int             clusterId,
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;
    S9sNode        master;
    S9sNode        slave;
    int            nSlaves  = 0;
    int            nMasters = 0;
    
    /*
     * Finding the slave and the master.
     */
    for (uint idx = 0; idx < hosts.size(); ++idx)
    {
        const S9sNode &node = hosts[idx].toNode();
        bool           isMaster = node.property("master").toBoolean();
        bool           isSlave = node.property("slave").toBoolean();

        S9S_DEBUG("[%02u] %5s %s", 
                idx, 
                isMaster ? "true" : "false",
                isSlave  ? "true" : "false",
                STR(node.hostName()));

        if (isSlave)
        {
            nSlaves += 1;
            slave = hosts[idx].toNode();
        }

        if (isMaster)
        {
            nMasters += 1;
            master = hosts[idx].toNode();
        }
    }

    if (nSlaves != 1 || nMasters != 1)
    {
        PRINT_ERROR("To add a slave to an existing master one slave and"
                " one master has to be specified.");
        return false;
    }

    // The job_data describing the cluster.
    jobData["master_address"]   = master.hostName();
    jobData["slave_address"]    = slave.hostName();

    if (slave.hasPort())
        jobData["port"]         = slave.port();

    jobData["install_software"] = true;
    jobData["disable_firewall"] = true;
    jobData["disable_selinux"]  = true;
   
    // The jobspec describing the command.
    jobSpec["command"]    = "add_replication_slave";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Add Slave to Cluster";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"] = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = clusterId;
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    } else {
        PRINT_ERROR(
                "Either the --cluster-id or the --cluster-name command line "
                "option has to be provided.");
        return false;
    }
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::addHaProxy(
        const int             clusterId,
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList haProxyNodes;
    S9sVariantList otherNodes;
    S9sString      nodeAddresses;
    bool           retval;

    S9sNode::selectByProtocol(hosts, haProxyNodes, otherNodes, "haproxy");

    if (haProxyNodes.size() != 1u)
    {
        PRINT_ERROR(
                "To add a HaProxy one needs to specify exactly"
                " one HaProxy node.");

        return false;
    }

    // The job_data describing the cluster.
    jobData["action"]          = "setupHaProxy";
    jobData["haproxy_address"] = haProxyNodes[0].toNode().hostName();
    
    for (uint idx = 0u; idx < otherNodes.size(); ++idx)
    {
        int       port;
        S9sNode   node;
        S9sString tmp;

        node = otherNodes[idx].toNode();
        port = node.hasPort() ? node.port() : 3306;

        tmp.sprintf("%s:%d:%s", STR(node.hostName()), port, "active");

        if (!nodeAddresses.empty())
            nodeAddresses += ";";

        nodeAddresses += tmp;
    }

    if (!nodeAddresses.empty())
        jobData["node_addresses"] = nodeAddresses;

    // The jobspec describing the command.
    jobSpec["command"]    = "haproxy";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Add HaProxy to Cluster";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"] = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to create a job that will add a proxysql host to the cluster.
 */
bool
S9sRpcClient::addProxySql(
        const int             clusterId,
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList proxyNodes;
    S9sVariantList otherNodes;
    bool           retval;

    S9sNode::selectByProtocol(hosts, proxyNodes, otherNodes, "proxysql");
    if (proxyNodes.size() != 1u)
    {
        PRINT_ERROR(
                "To add a ProxySql one needs to specify exactly"
                " one ProxySql node.");

        return false;
    }

    if (otherNodes.size() > 1u)
    {
        PRINT_ERROR(
                "Specifying extra nodes when adding ProxySql is not"
                " supported.");

        return false;
    }

    // The job_data describing the cluster.
    jobData["action"]   = "setupProxySql";
    jobData["hostname"] = proxyNodes[0].toNode().hostName();
    
    /*
     * Some information:
        "db_database": "*.*",
        "db_password": "cmon",
        "db_privs": "",
        "db_username": "cmon",
    */
    printf("WARNING: admin/admin\n");
    printf("WARNING: proxy-monitor/proxy-monitor\n");

    jobData["admin_user"]       = "admin";
    jobData["admin_password"]   = "admin",
    jobData["monitor_user"]     = "proxy-monitor";
    jobData["monitor_password"] = "proxysql-monitor";


    // The jobspec describing the command.
    jobSpec["command"]    = "proxysql";
    jobSpec["job_data"]   = jobData;
    
    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Add ProxySQL to Cluster";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"] = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to create a job that will add a maxscale host to the cluster.
 */
bool
S9sRpcClient::addMaxScale(
        const int             clusterId,
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sVariantList maxScaleNodes;
    S9sVariantList otherNodes;
    S9sString      nodeAddresses;
    bool           retval;

    S9sNode::selectByProtocol(hosts, maxScaleNodes, otherNodes, "maxscale");

    if (maxScaleNodes.size() != 1u)
    {
        PRINT_ERROR(
                "To add a MaxScale one needs to specify exactly"
                " one MaxScale node.");

        return false;
    }

    for (uint idx = 0u; idx < otherNodes.size(); ++idx)
    {
        int       port;
        S9sNode   node;
        S9sString tmp;

        node = otherNodes[idx].toNode();
        port = node.hasPort() ? node.port() : 3306;

        tmp.sprintf("%s:%d", STR(node.hostName()), port);

        if (!nodeAddresses.empty())
            nodeAddresses += ";";

        nodeAddresses += tmp;
    }

    if (!nodeAddresses.empty())
        jobData["node_addresses"] = nodeAddresses;
    
    // The job_data describing the cluster.
    jobData["action"]   = "setupMaxScale";
    // FIXME: Onc it is this, then that.
    //jobData["hostname"] = maxScaleNodes[0].toNode().hostName();
    jobData["server_address"] = maxScaleNodes[0].toNode().hostName();
    
    // The jobspec describing the command.
    jobSpec["command"]    = "maxscale";
    jobSpec["job_data"]   = jobData;
    if (!options->schedule().empty())
        job["scheduled"] = options->schedule(); 

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Add MaxScale to Cluster";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"] = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \param hosts the hosts that will be removed from the cluster (variant list
 *   with S9sNode elements).
 * \returns true if the request was sent and the reply was received (even if the
 *   reply is an error notification).
 *
 * This function will create a "removeNode" job on the controller.
 */
bool
S9sRpcClient::removeNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts = options->nodes();
    S9sNode        host;
    S9sString      hostName, title;
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;

    if (hosts.empty())
    {
        PRINT_ERROR(
                "Node list is empty while removing node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    if (hosts.size() != 1u)
    {
        PRINT_ERROR("Remove node is currently implemented only for one node.");
        return false;
    }
    
    host     = hosts[0].toNode();
    hostName = host.hostName();
    title.sprintf("Remove '%s' from the Cluster", STR(hostName));

    // The job_data describing the cluster.
    jobData["host"]       = hostName;
    if (host.hasPort())
        jobData["port"]   = host.port();
   
    // The jobspec describing the command.
    jobSpec["command"]    = "removenode";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"] = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request was sent and the reply was received (even if the
 *   reply is an error notification).
 *
 * This function will stop the cluster by creating a "stop_cluster" job.
 */
bool
S9sRpcClient::stopCluster()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sString      title;
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;
    
    title = "Stopping Cluster";

    // The job_data describing the cluster.
    jobData["force"]               = false;
    jobData["stop_timeout"]        = 1800;
    jobData["maintenance_minutes"] = 0;
    jobData["reason"]              = "Cluster is stopped.";
    
    // The jobspec describing the command.
    jobSpec["command"]    = "stop_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule();

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This method will create and send a start_cluster job.
 */
bool
S9sRpcClient::startCluster()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sString      title;
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;
   
    // The job_data
    if (!options->donor().empty())
        jobData["donor_address"] = options->donor();

    // The jobspec describing the command.
    jobSpec["command"]    = "start_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Starting Cluster";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function will create and send a job to start a node of a cluster.
 */
bool
S9sRpcClient::startNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("To start a node exactly one node must be specified.");
        return false;
    } else {
        node = hosts[0].toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["hostname"]   = node.hostName();
    
    if (node.hasPort())
        jobData["port"]   = node.port();
     
    // The jobspec describing the command.
    jobSpec["command"]    = "start";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Starting Node";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function will create and send a job to stop a node of a cluster.
 */
bool
S9sRpcClient::stopNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("To stop a node exactly one node must be specified.");
        return false;
    } else {
        node = hosts[0].toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["hostname"]   = node.hostName();
    
    if (node.hasPort())
        jobData["port"]   = node.port();
     
    if (options->force())
        jobData["force_stop"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "stop";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Stopping Node";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * This function will create and send a job to stop and then start a node of a
 * cluster.
 */
bool
S9sRpcClient::restartNode()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hosts     = options->nodes();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    S9sNode        node;
    bool           retval;
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("To restart a node exactly one node must be specified.");
        return false;
    } else {
        node = hosts[0].toNode();
    }
    
    // The job_data describing the job itself.
    jobData["clusterid"]  = clusterId;
    jobData["hostname"]   = node.hostName();
    
    if (node.hasPort())
        jobData["port"]   = node.port();
     
    if (options->force())
        jobData["force_stop"] = true;

    // The jobspec describing the command.
    jobSpec["command"]    = "restart";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Restarting Node";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}


/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request was sent and the reply was received (even if the
 *   reply is an error notification).
 *
 * This function will create a "remove_cluster" job that will eventually drop
 * the cluster from the cmon controller.
 */
bool
S9sRpcClient::dropCluster()
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sString      title;
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri = "/v2/jobs/";
    bool           retval;
    
    title = "Remove Cluster";

    // The job_data describing the cluster that will be deleted.
    jobData["clusterid"]  = clusterId;
    
    // The jobspec describing the command.
    jobSpec["command"]    = "remove_cluster";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = title;
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule(); 

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = 0;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::checkHosts()
{
    getSshCredentials();
    S9sString      uri = "/v2/discovery/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList hosts     = options->nodes();
   
    if (hosts.empty())
        return true;

    request["operation"]      = "checkHosts";
    request["nodes"]          = nodesField(hosts);
    request["check_if_already_registered"] = true;
    //request["check_ssh_sudo"] = true;
    
#if 0
    S9sVariantMap credentials;

    credentials["class_name"]      = "CmonSshCredentials";
    credentials["user_name"]       = "pipas";
    credentials["password"]        = "";
    credentials["public_key_file"] = "./configs/testing_id_rsa";
    //credentials["port"]            = ;
    //credentials["timeout"]         = ;
    //credentials["tty_for_sudo"]    = ;
    request["ssh_credentials"]     = credentials;
#endif

#if 1 
    if (options->hasClusterIdOption())
        request["cluster_id"]   = options->clusterId();

    if (options->hasClusterNameOption())
        request["cluster_name"] = options->clusterName();
#endif
    return executeRequest(uri, request);
}

/**
 * This is where we send the "registerServers" request that will notify the
 * controller about a container server we want to use.
 */
bool
S9sRpcClient::registerServers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
   
    request["operation"]      = "registerServers";
    request["servers"]        = serversField(servers);
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::moveInTree()
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    
    if (options->nExtraArguments() != 2)
    {
        PRINT_ERROR(
                "The --move option requires two command line arguments: "
                "the source path and the target path.");

        return false;
    }
   
   
    request["operation"]      = "move";
    request["source_path"]    = options->extraArgument(0u);
    request["target_path"]    = options->extraArgument(1u);
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::deleteFromTree()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    
    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --delete option requires one command line argument: "
                "the full path of the object to delete.");

        return false;
    }
   
   
    request["operation"]      = "delete";
    request["path"]           = options->extraArgument(0u);
    
    return executeRequest(uri, request);
}

/**
 * Sends the getAcl request to the controller.
 */
bool
S9sRpcClient::getAcl()
{
    S9sString        uri = "/v2/host/";
    S9sVariantMap    request;
    S9sOptions      *options   = S9sOptions::instance();
    S9sVariantList   servers   = options->servers();

    if (!servers.empty())
    {
        uri = "/v2/host/";
        request["servers"] = serversField(servers);
    } else if (options->nExtraArguments() == 1)
    {
        uri = "/v2/tree/";
        request["path"] = options->extraArgument(0u);
    } else {
        PRINT_ERROR(
                "The --get-acl option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
    request["operation"]      = "getAcl";
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::startInTree()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    
    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --start option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
   
    request["operation"]      = "start";
    request["path"]           = options->extraArgument(0u);
    
    return executeRequest(uri, request);
}


bool
S9sRpcClient::addAcl()
{
    S9sString      uri = "/v2/tree/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      aclString = options->acl();

    if (options->nExtraArguments() != 1)
    {
        PRINT_ERROR(
                "The --add-acl option requires one command line argument: "
                "the path of the object.");

        return false;
    }
   
    if (aclString.empty())
    {
        PRINT_ERROR("The --add-acl requires the --acl=STRING option.");

        return false;
    }
   
    request["operation"]      = "addAcl";
    request["path"]           = options->extraArgument(0u);
    request["acl"]            = aclString;

    return executeRequest(uri, request);
}


/**
 * Unregisters one or more CmonContainerServer in one step.
 */
bool
S9sRpcClient::unregisterServers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
   
    request["operation"]      = "unregisterServers";
    request["servers"]        = serversField(servers);
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::unregisterHost()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
    S9sVariantList hosts;
    
    /*
     * Finding the host(s).
     */
    hosts = options->nodes();
    if (hosts.empty())
    {
        PRINT_ERROR(
            "Node list is empty while unregistering nodes.\n"
            "Use the --nodes command line option to provide a node."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    } else if (hosts.size() > 1u)
    {
        PRINT_ERROR(
            "Only one node can be unregister at a time."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return false;
    }

    request["operation"]      = "unregisterHost";
    request["host"]           = hosts[0];
    //request["dry_run"]        = true;

    if (options->hasClusterIdOption())
        request["cluster_id"]   = options->clusterId();

    if (options->hasClusterNameOption())
        request["cluster_name"] = options->clusterName();
    
    return executeRequest(uri, request);
}



bool
S9sRpcClient::getContainers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
   
    request["operation"]      = "getContainers";

    if (!servers.empty())
        request["servers"]    = serversField(servers);
    
    return executeRequest(uri, request);
}

/**
 * Gets the servers from the controller.
 */
bool
S9sRpcClient::getServers()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
   
    request["operation"]      = "getServers";

    if (!servers.empty())
        request["servers"]    = serversField(servers);
    
    if (options->isRefreshRequested())
        request["refresh_now"] = true;
    
    return executeRequest(uri, request);
}

/**
 * Sends a request to create a container.
 */
bool
S9sRpcClient::createContainer()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options    = S9sOptions::instance();
    S9sVariantList servers    = options->servers();
   
    if (options->nExtraArguments() == 1)
    {
        S9sVariantMap  container;

        container["class_name"]   = "CmonContainer";
        container["alias"]        = options->extraArgument(0);
    
        request["container"]      = container;
    } else if (options->nExtraArguments() == 2)
    {
        PRINT_ERROR("Currently only one container can be created at a time.");
        return false;
    }

    request["operation"]      = "createContainer";
    //request["start"]          = true;
    //request["wait_for_start"] = true;
    //request["check_ssh"]      = true;

    if (!servers.empty())
        request["servers"] = serversField(servers);
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::deleteContainer()
{
    S9sString      uri = "/v2/host/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList servers   = options->servers();
    S9sVariantMap  container;

    container["class_name"]  = "CmonContainer";
    container["alias"]       = options->extraArgument(0);

    request["operation"]     = "deleteContainer";
    request["container"]     = container;

    if (!servers.empty())
        request["servers"] = serversField(servers);
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::getSupportedClusterTypes()
{
    getSshCredentials();
    S9sString      uri = "/v2/discovery/";
    S9sVariantMap  request;
   
    request["operation"]     = "getSupportedClusterTypes";
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::checkClusterName()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/discovery/";
    S9sVariantMap  request;
   
    request["operation"]        = "checkClusterName";
    request["new_cluster_name"] = options->clusterName();
    
    return executeRequest(uri, request);
}

bool
S9sRpcClient::getSshCredentials()
{
    S9sString      uri = "/v2/discovery/";
    S9sVariantMap  request;
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantList hosts     = options->nodes();
   
    if (hosts.empty())
        return true;

    request["operation"]      = "getSshCredentials";
    
    if (options->hasClusterIdOption())
        request["cluster_id"]   = options->clusterId();

    if (options->hasClusterNameOption())
        request["cluster_name"] = options->clusterName();
    
    return executeRequest(uri, request);
}

/**
 * Places a job that will create a new backup.
 *
 * Using this function to place a scheduled backup is not working because it is
 * incompatible with the backend.
 */
bool
S9sRpcClient::createBackup()
{
    S9sOptions     *options      = S9sOptions::instance();
    int             clusterId    = options->clusterId();
    S9sString       clusterName  = options->clusterName();
    S9sVariantList  hosts        = options->nodes();
    S9sString       backupMethod = options->backupMethod();
    S9sString       backupDir    = options->backupDir();
    S9sString       schedule     = options->schedule();
    S9sString       databases    = options->databases();
    S9sNode         backupHost;
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";
    bool            retval;

    if (!options->hasClusterIdOption() && !options->hasClusterNameOption())
    {
        PRINT_ERROR("The cluster ID or the cluster name must be specified.");
        return false;
    }

    if (hosts.size() != 1u)
    {
        PRINT_ERROR("To create a new backup one node must be specified.");
        return false;
    }

    backupHost = hosts[0].toNode();


    // The job_data describing how the backup will be created.
    jobData["hostname"]          = backupHost.hostName();
    jobData["description"]       = "Backup created by s9s-tools.";

    if (backupHost.hasPort())
        jobData["port"]          = backupHost.port();

    if (!backupDir.empty())
        jobData["backupdir"]     = backupDir;

    if (!backupMethod.empty())
        jobData["backup_method"] = backupMethod;
    
    if (!databases.empty())
        jobData["include_databases"] = databases;

    if (options->noCompression())
        jobData["compression"]   = false;

    if (options->usePigz())
        jobData["use_pigz"]      = true;

    if (options->onNode())
        jobData["cc_storage"]    = false;

    if (options->hasParallellism())
        jobData["xtrabackup_parallellism"] = options->parallellism();

    // The jobspec describing the command.
    jobSpec["command"]    = "backup";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Create Backup";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule();

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;

    if (options->hasClusterIdOption())
        request["cluster_id"] = clusterId;
    else if (options->hasClusterNameOption())
        request["cluster_name"] = clusterName;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Creates a job to restore a pre-existing backup.
 */
bool
S9sRpcClient::restoreBackup()
{
    S9sOptions     *options = S9sOptions::instance();
    int             clusterId = options->clusterId();
    int             backupId  = options->backupId();
    S9sString       backupMethod = options->backupMethod();
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/v2/jobs/";
    bool            retval;

    // The job_data describing how the backup will be created.
    jobData["backupid"]   = backupId;
    jobData["bootstrap"]  = true;
    if (!options->nodes().empty())
    {
        // on which node we want to restore the backup
        S9sNode   node    = options->nodes()[0].toNode();
        S9sString address = node.hostName();

        // lets include also portNum if specified
        if (node.hasPort())
            address.sprintf("%s:%d", STR(node.hostName()), node.port());

        jobData["server_address"] = address;
    }

    // The jobspec describing the command.
    jobSpec["command"]    = "restore_backup";
    jobSpec["job_data"]   = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]     = "CmonJobInstance";
    job["title"]          = "Restore Backup";
    job["job_spec"]       = jobSpec;

    if (!options->schedule().empty())
        job["scheduled"]  = options->schedule();

    // The request describing we want to register a job instance.
    request["operation"]  = "createJobInstance";
    request["job"]        = job;
    request["cluster_id"] = clusterId;
    
    retval = executeRequest(uri, request);
    
    return retval;
}

/**
 * \param clusterId The cluster to read or <= 0 to read all the clusters
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * The method that gets the list of backups from the server.
 */
bool
S9sRpcClient::getBackups(
        const int clusterId)
{
    S9sString      uri = "/v2/backup/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getBackups";
    request["ascending"] = true;

    if (clusterId > 0)
        request["cluster_id"] = clusterId;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \param backupId The ID of the backup to delete.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 */
bool
S9sRpcClient::deleteBackupRecord(
        const ulonglong backupId)
{
    S9sString      uri = "/v2/backup/";
    S9sVariantMap  request;
    S9sVariantMap  backupMap;
    bool           retval;

    backupMap["backup_id"]   = backupId;
    request["operation"]     = "deleteBackupRecord";
    request["backup_record"] = backupMap;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to add a new account to the cluster. With this account the user
 * can work on the production database.
 */
bool
S9sRpcClient::createAccount()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;
    S9sAccount     account;
    bool           retval;

    account = options->account();
    account.setWithDatabase(options->withDatabase());
    account.setGrants(options->privileges());

    request["operation"]  = "createAccount";
    request["account"]    = account;

    if (options->hasClusterIdOption())
        request["cluster_id"] = options->clusterId();

    if (!options->clusterName().empty())
        request["cluster_name"] = options->clusterName();

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::getAccounts()
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getAccounts";
    
    if (options->limit() >= 0)
        request["limit"] = options->limit();

    if (options->offset() >= 0)
        request["offset"] = options->offset();

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getRepositories()
{
    #if 1
    static int testJob = 0;
    if (testJob == 0)
    {
        createLocalRepository(1, "galera", "percona", "5.6", "precise");
        ++testJob;
    }
    #endif

    S9sOptions    *options   = S9sOptions::instance();
    S9sString      uri = "/v2/repositories/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getRepositories";

    if (options->hasClusterIdOption())
    {
        request["cluster_id"] = options->clusterId();
    } else if (options->hasClusterNameOption())
    {
        request["cluster_name"] = options->clusterName();
    }

    return executeRequest(uri, request);
}

bool
S9sRpcClient::getSupportedSetups()
{
    S9sString      uri = "/v2/repositories/";
    S9sVariantMap  request;

    // Building the request.
    request["operation"]  = "getSupportedSetups";

    return executeRequest(uri, request);
}

/**
 * \param account The account for which we are granting privileges.
 * \param privileges The privileges in GSL language.
 * \returns True if the request sent and the reply received even if that reply
 *   is an error reply.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to grant rights for an account that already exists.
 *
 * FIXME: We need to handle the cluster name and cluster ID smarter than this.
 */
bool
S9sRpcClient::grantPrivileges(
        const S9sAccount &account,
        const S9sString  &privileges)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "grantPrivileges";
    request["account"]    = account;
    request["privileges"] = privileges;

    if (options->hasClusterIdOption())
        request["cluster_id"] = options->clusterId();

    if (!options->clusterName().empty())
        request["cluster_name"] = options->clusterName();

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to delete an account from the cluster. 
 */
bool
S9sRpcClient::deleteAccount()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;
    S9sAccount     account;
    bool           retval;

    account = options->account();
    // We don't need these, do we?
    //account.setWithDatabase(options->withDatabase());
    //account.setGrants(options->privileges());

    request["operation"]  = "deleteAccount";
    request["account"]    = account;

    if (options->hasClusterIdOption())
        request["cluster_id"] = options->clusterId();

    if (!options->clusterName().empty())
        request["cluster_name"] = options->clusterName();

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::createDatabase()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/clusters/";
    S9sVariantMap  request;
    S9sVariantMap  database;
    bool           retval;

    database["class_name"]    = "CmonDataBase";
    database["database_name"] = options->dbName();

    request["operation"]      = "createDatabase";
    request["database"]       = database;

    if (options->hasClusterIdOption())
        request["cluster_id"] = options->clusterId();

    if (!options->clusterName().empty())
        request["cluster_name"] = options->clusterName();

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::saveScript(
        S9sString remoteFileName,
        S9sString content)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/imperative/";
    S9sVariantMap  request;

    request["operation"]      = "saveScript";
    request["filename"]       = remoteFileName;
    request["content"]        = content;

    if (options->hasClusterIdOption())
        request["cluster_id"] = options->clusterId();

    if (!options->clusterName().empty())
        request["cluster_name"] = options->clusterName();

    return executeRequest(uri, request);
}

bool
S9sRpcClient::executeExternalScript(
        S9sString localFileName,
        S9sString content,
        S9sString arguments)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/imperative/";
    S9sVariantMap  request;

    request["operation"]      = "executeExternalScript";
    request["filename"]       = localFileName;
    request["content"]        = content;
    request["arguments"]      = arguments;

    if (options->hasClusterIdOption())
        request["cluster_id"] = options->clusterId();

    if (!options->clusterName().empty())
        request["cluster_name"] = options->clusterName();

    return executeRequest(uri, request);
}


bool
S9sRpcClient::executeScript(
        S9sString remoteFileName,
        S9sString arguments)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/imperative/";
    S9sVariantMap  request;

    request["operation"]      = "executeScript";
    request["filename"]       = remoteFileName;
    request["arguments"]      = arguments;

    if (options->hasClusterIdOption())
        request["cluster_id"] = options->clusterId();

    if (!options->clusterName().empty())
        request["cluster_name"] = options->clusterName();

    return executeRequest(uri, request);
}

bool
S9sRpcClient::removeScript(
        S9sString remoteFileName)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/imperative/";
    S9sVariantMap  request;

    request["operation"]      = "removeScript";
    request["filename"]       = remoteFileName;

    if (options->hasClusterIdOption())
        request["cluster_id"] = options->clusterId();

    if (!options->clusterName().empty())
        request["cluster_name"] = options->clusterName();

    return executeRequest(uri, request);
}

bool
S9sRpcClient::treeScripts()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/imperative/";
    S9sVariantMap  request;

    request["operation"]      = "dirTree";

    if (options->hasClusterIdOption())
        request["cluster_id"] = options->clusterId();

    if (!options->clusterName().empty())
        request["cluster_name"] = options->clusterName();

    return executeRequest(uri, request);
}

S9sVariantMap
S9sRpcClient::createUserRequest(
        const S9sUser   &user,
        const S9sString &newPassword,
        bool             createGroup)
{
    S9sVariantMap  request;

    request["operation"]    = "createUser";
    request["user"]         = user.toVariantMap();
    request["create_group"] = createGroup;

    if (!newPassword.empty())
        request["new_password"] = newPassword;

    return request;
}

bool
S9sRpcClient::createUser(
        const S9sUser   &user,
        const S9sString &newPassword,
        bool             createGroup)
{
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    
    request = createUserRequest(user, newPassword, createGroup);

    return executeRequest(uri, request);
}

bool
S9sRpcClient::addToGroup(
        const S9sUser     &user,
        const S9sString   &groupName,
        bool               replacePrimaryGroup)
{
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    
    request["operation"]    = "addToGroup";
    request["user"]         = user.toVariantMap();
    request["group_name"]   = groupName;
    request["replace_primary_group"] = replacePrimaryGroup;
    
    return executeRequest(uri, request);
}


/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * The method that gets the list of users from the server.
 */
bool
S9sRpcClient::getUsers()
{
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getUsers";

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::getGroups()
{
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getGroups";

    retval = executeRequest(uri, request);

    return retval;
}


/**
 * \returns true if the request sent and a reply is received (even if the reply
 *   is an error message).
 *
 * This method is used to modify an existing Cmon User on the controller.
 */
bool
S9sRpcClient::setUser()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    S9sVariantMap  properties;
    bool           retval;

    if (options->nExtraArguments() > 1)
    {
        PRINT_ERROR("Only one user can be modified at once.");
        return false;
    }

    properties["class_name"] = "CmonUser";

    if (options->nExtraArguments() > 0)
    {
        properties["user_name"]  = options->extraArgument(0);
    } else {
        properties["user_name"] = options->userName();
    }

    if (!options->firstName().empty())
        properties["first_name"] = options->firstName();
    
    if (!options->lastName().empty())
        properties["last_name"] = options->lastName();
    
    if (!options->title().empty())
        properties["title"] = options->title();
    
    if (!options->emailAddress().empty())
        properties["email_address"] = options->emailAddress();

    request["operation"]  = "setUser";
    request["user"]       = properties;

    retval = executeRequest(uri, request);

    return retval;
}

bool
S9sRpcClient::getKeys()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    S9sVariantMap  properties;

    if (options->nExtraArguments() > 1)
    {
        PRINT_ERROR("More than one user when getting keys.");
        return false;
    }
    
    properties["class_name"] = "CmonUser";
    if (options->nExtraArguments() > 0)
    {
        properties["user_name"]  = options->extraArgument(0);
    } else {
        properties["user_name"] = options->userName();
    }
    
    request["operation"]  = "getKeys";
    request["user"]       = properties;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::addKey()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    S9sVariantMap  userMap, keyMap;
    S9sString      keyFileName = options->publicKeyPath();
    S9sFile        keyFile(keyFileName);
    S9sString      key;
    bool           success;

    /*
     * The user.
     */
    if (options->nExtraArguments() > 1)
    {
        PRINT_ERROR("More than one user when getting keys.");
        return false;
    }
    
    userMap["class_name"] = "CmonUser";
    if (options->nExtraArguments() > 0)
    {
        userMap["user_name"]  = options->extraArgument(0);
    } else {
        userMap["user_name"] = options->userName();
    }

    /*
     * The key.
     */
    if (keyFileName.empty())
    {
        PRINT_ERROR("The public key file was not specified.");
        PRINT_ERROR("Use the --public-key-path command line option to "
                "specify the public key file.");
        return false;
    }

    success = keyFile.readTxtFile(key);
    if (!success)
    {
        PRINT_ERROR("%s", STR(keyFile.errorString()));
    }

    if (key.empty())
    {
        PRINT_ERROR("Invalid key in file '%s'.", STR(keyFileName));
        return false;
    }
    
    keyMap["key"]          = key;
    keyMap["name"]         = options->publicKeyName();

    request["operation"]   = "addKey";
    request["user"]        = userMap;
    request["public_key"]  = keyMap;

    return executeRequest(uri, request);
}

bool
S9sRpcClient::setPassword()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/v2/users/";
    S9sVariantMap  request;
    S9sVariantMap  properties;
    bool           changingSelf = false;

    if (options->nExtraArguments() > 1)
    {
        PRINT_ERROR("Only one user can be modified at once.");
        return false;
    }

    //
    //
    //
    properties["class_name"] = "CmonUser";
    if (options->nExtraArguments() > 0)
    {
        properties["user_name"]  = options->extraArgument(0);
    } else {
        properties["user_name"] = options->userName();
        changingSelf = true;
    }

    //
    // 
    //
    request["operation"]  = "changePassword";
    request["user"]       = properties;

    if (options->hasOldPassword())
    {
        request["old_password"] = options->oldPassword();
    } else if (changingSelf && options->hasPassword())
    {
        request["old_password"] = options->password();
    }

    if (options->hasNewPassword())
        request["new_password"] = options->newPassword();
    
    return executeRequest(uri, request);
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * The function to create maintenance as it is defined in the S9sOptions
 * singleton.
 */
bool 
S9sRpcClient::createMaintenance()
{
    S9sOptions    *options = S9sOptions::instance();
    bool           success;

    /*
     * Two ways: host maintenance or cluster maintenance.
     */
    if (options->hasClusterIdOption())
    {
        success = createMaintenance(
                options->clusterId(), options->start(), options->end(),
                options->reason());
    } else {
        success = createMaintenance(
                options->nodes(), options->start(), options->end(),
                options->reason());
    }

    return success;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Overloaded version of the function that create a maintence period as defined
 * in the function arguments.
 */
bool 
S9sRpcClient::createMaintenance(
        const S9sVariantList &hosts,
        const S9sString      &start,
        const S9sString      &end,
        const S9sString      &reason)
{
    S9sString      uri = "/v2/maintenance/";
    S9sVariantMap  request;
    bool           retval;

    if (hosts.size() != 1)
    {
        PRINT_ERROR(
                "To create a maintenance period one"
                " hostname has to be provided.");
        return false;
    }
    
    request["operation"] = "addMaintenance";
    request["hostname"]  = hosts[0].toNode().hostName();
    request["initiate"]  = start;
    request["deadline"]  = end;
    request["reason"]    = reason;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Overloaded version of the function that create a maintence period as defined
 * in the function arguments.
 */
bool 
S9sRpcClient::createMaintenance(
        const int            &clusterId,
        const S9sString      &start,
        const S9sString      &end,
        const S9sString      &reason)
{
    S9sString      uri = "/v2/maintenance/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "addMaintenance";
    request["cluster_id"] = clusterId;
    request["initiate"]   = start;
    request["deadline"]   = end;
    request["reason"]     = reason;

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 */
bool
S9sRpcClient::deleteMaintenance()
{
    S9sOptions    *options = S9sOptions::instance();

    return deleteMaintenance(options->uuid());
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Overloaded function that will delete a maintence as it is defined in the
 * command line argument.
 */
bool
S9sRpcClient::deleteMaintenance(
        const S9sString &uuid)
{
    S9sString      uri = "/v2/maintenance/";
    S9sVariantMap  request;
    bool           retval;

    if (uuid.empty())
    {
        PRINT_ERROR("Missing UUID.");
        PRINT_ERROR("Use the --uuid command line option to provide the UUID.");
        return false;
    }
    
    request["operation"] = "removeMaintenance";
    request["UUID"]      = uuid;

    //request["hostname"]  = hosts[0].toNode().hostName();

    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * Get the maintenance periods from the server.
 */
bool
S9sRpcClient::getMaintenance()
{
    S9sString      uri = "/v2/maintenance/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"] = "getMaintenance";
    //request["including_hosts"] = "192.168.1.101;192.168.1.102;192.168.1.104";
    
    retval = executeRequest(uri, request);

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 * 
 */
bool
S9sRpcClient::executeRequest(
        const S9sString &uri,
        S9sVariantMap   &request)
{
    S9sDateTime    now = S9sDateTime::currentDateTime();
    S9sString      timeString = now.toString(S9sDateTime::TzDateTimeFormat);

    request["request_created"] = timeString;
    request["request_id"]      = ++m_priv->m_requestId;

    return doExecuteRequest(uri, request.toString());
}

/**
 * \param uri the file path part of the URL where we send the request
 * \param payload the JSON request string
 * \returns true if everything is ok, false on error.
 */
bool
S9sRpcClient::doExecuteRequest(
        const S9sString &uri,
        const S9sString &payload)
{
    S9sOptions  *options = S9sOptions::instance();    
    S9sDateTime  replyReceived;
    S9sString    header;
    ssize_t      readLength;
   
    m_priv->m_jsonReply.clear();
    m_priv->m_reply.clear();

    if (!m_priv->connect())
    {
        PRINT_VERBOSE ("Connection failed: %s", STR(m_priv->m_errorString));
        return false;
    }
        
    /*
     * Printing the request we are sending.
     */
    if (options->isJsonRequested() && options->isVerbose())
    {
        printf("Preparing to send request on %s: \n%s\n", 
                STR(uri), STR(payload));
    }

    header.sprintf(
        "POST %s HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "User-Agent: s9s-tools/1.0\r\n"
        "Connection: close\r\n"
        "Accept: application/json\r\n"
        "Transfer-Encoding: identity\r\n"
        "%s"
        "Content-Type: application/json\r\n"
        "Content-Length: %u\r\n"
        "\r\n",
        STR(uri),
        STR(m_priv->m_hostName),
        m_priv->m_port,
        STR(m_priv->cookieHeaders()),
        payload.length());

    /*
     * Sending the HTTP request header.
     */
    if (m_priv->write(STR(header), header.length()) < 0)
    {
        // we shall use m_priv->m_errorString TODO
        S9S_DEBUG("Error writing socket: %m");

        // priv shall do this:
        m_priv->m_errorString.sprintf("Error writing socket: %m");
        m_priv->close();

        return false;
    }

    /*
     * Sending the JSON payload.
     */
    if (!payload.empty())
    {
        if (m_priv->write(STR(payload), payload.length()) < 0)
        {
            // we shall use m_priv->m_errorString TODO
            m_priv->m_errorString.sprintf(
                    "Error writing socket: %m");
       
            m_priv->close();
            return false;
        } else {
            if (options->isJsonRequested() && options->isVerbose())
            {
                printf("Sent request.\n");
            }
        }
    }

    /*
     * Reading the reply from the server.
     */
    replyReceived = S9sDateTime::currentDateTime();

    m_priv->clearBuffer();
    readLength = 0;
    do
    {
        m_priv->ensureHasBuffer(m_priv->m_dataSize + READ_SIZE);

        readLength = m_priv->read(
                m_priv->m_buffer + m_priv->m_dataSize, 
                READ_SIZE - 1);

        if (readLength > 0)
            m_priv->m_dataSize += readLength;
    } while (readLength > 0);

    // Closing the buffer with a null terminating byte.
    m_priv->ensureHasBuffer(m_priv->m_dataSize + 1);
    m_priv->m_buffer[m_priv->m_dataSize] = '\0';
    m_priv->m_dataSize += 1;
    S9S_DEBUG("reply: '%s'", m_priv->m_buffer); 


    // Closing the socket.
    m_priv->close();
    
    if (m_priv->m_dataSize > 1)
    {
        // Lets parse the cookie/HTTP session info from server reply
        m_priv->parseHeaders();

        char *tmp = strstr(m_priv->m_buffer, "\r\n\r\n");

        if (tmp)
        {
            m_priv->m_jsonReply = (tmp + 4);

            if (options->isJsonRequested() && options->isVerbose())
                printf("Reply: \n%s\n", STR(m_priv->m_jsonReply));
        }
    } else {
        m_priv->m_errorString.sprintf(
                "Error reading socket (%s:%d TLS: %s): %m",
                STR(m_priv->m_hostName), m_priv->m_port,
                m_priv->m_useTls ? "yes" : "no");

        return false;
    }

    if (!m_priv->m_reply.parse(STR(m_priv->m_jsonReply)))
    {
        PRINT_ERROR("Error parsing JSON reply.");
        m_priv->m_errorString.sprintf("Error parsing JSON reply.");
        return false;
    } else {
        m_priv->m_reply["reply_received"] = 
            replyReceived.toString(S9sDateTime::TzDateTimeFormat);
    }

    //printf("-> \n%s\n", STR(m_priv->m_reply.toString()));
    return true;
}

S9sVariant
S9sRpcClient::topologyField(
        const S9sVariantList &nodes)
{
    S9sVariantList masterSlaveLinks;
    S9sVariantMap  topology;
    S9sVariant     retval;
    S9sString      masterHostName;

    for (uint idx = 0u; idx < nodes.size(); ++idx)
    {
        const S9sNode &node = nodes[idx].toNode();
        bool  isMaster      = node.property("master").toBoolean();
        bool  isSlave       = node.property("slave").toBoolean();

        // If the user did not provide information about the master/slave status
        // we consider the first node a master and the others slave.
        if (!isMaster && !isSlave)
        {
            isMaster = idx == 0u;
            isSlave  = !isMaster;
        }
        #if 0
        S9S_WARNING("%-20s %-6s", 
                STR(node.hostName()),
                isMaster ? "master" : (isSlave ? "slave" : "-"));
        #endif

        if (isMaster)
        {
            if (!masterHostName.empty())
            {
                S9sVariantMap map1, map2;

                map1[masterHostName]  = node.hostName();
                map2[node.hostName()] = masterHostName;

                masterSlaveLinks << map1;
                masterSlaveLinks << map2;
            }

            masterHostName = node.hostName();
        } else if (isSlave)
        {
            if (!masterHostName.empty())
            {
                S9sVariantMap map1;

                map1[masterHostName] = node.hostName();
                masterSlaveLinks << map1;
            }
        }
    }

    topology["master_slave_links"] = masterSlaveLinks;
    retval = topology;
    return retval;
}

S9sVariant
S9sRpcClient::nodesField(
        const S9sVariantList &nodes)
{
    S9sVariant     retval;

    retval = nodes;
    return retval;
}

S9sVariant
S9sRpcClient::serversField(
        const S9sVariantList &servers)
{
    S9sVariant     retval;

    retval = servers;
    return retval;
}
