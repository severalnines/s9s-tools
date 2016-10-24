/*
 * Severalnines Tools
 * Copyright (C) 2016  Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9srpcclient.h"
#include "s9srpcclient_p.h"

#include "S9sOptions"
#include "S9sNode"
#include "S9sRsaKey"

#include <cstring>
#include <cstdio>

//#define DEBUG
//#define WARNING
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
 * \param token a token to be used with the communication.
 * \param useTls if client must initiate TLS encryption to the server.
 *
 */
S9sRpcClient::S9sRpcClient(
        const S9sString &hostName,
        const int        port,
        const S9sString &token,
        const bool       useTls) :
    m_priv(new S9sRpcClientPrivate)
{
    m_priv->m_hostName = hostName;
    m_priv->m_port     = port;
    m_priv->m_token    = token;
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
 * \returns the human readable error string stored in the object.
 */
S9sString 
S9sRpcClient::errorString() const
{
    return m_priv->m_errorString;
}

bool
S9sRpcClient::authenticate()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRsaKey      rsa;
    S9sString      uri = "/0/auth";
    S9sVariantMap  request;
    bool           retval;

    S9sString      privKeyPath = options->privateKeyPath();

    if (privKeyPath.empty())
    {
        m_priv->m_errorString =
                "Private key not specified, authentication is not possible.";
        return false;
    }

    if (! rsa.loadKeyFromFile(privKeyPath))
    {
        m_priv->m_errorString.sprintf (
                "Could not load user private key: %s",
                STR(privKeyPath));
        return false;
    }

    /*
     * First request, 'login'
     */
    request = S9sVariantMap();
    request["operation"]    = "login";
    request["username"]     = options->authUsername();

    retval = executeRequest(uri, request.toString());
    if (!retval)
        return false;

    S9sRpcReply loginReply = reply();

    S9sString signature;
    S9sString challenge = loginReply["challenge"].toString();

    // create an RSA-SHA256 signature using user's privkey
    rsa.signRsaSha256(challenge, signature);

    request = S9sVariantMap();
    request["operation"]    = "response";
    request["signature"]    = signature;

    retval = executeRequest(uri, request.toString());
    if (!retval)
        return false;

    /*
     * if reply doesn't contain an error
     * and we are ok, auth succeed
     */
    m_priv->m_errorString = reply().errorString ();
    return reply().isOk();
}

/**
 * The method that sends the "getAllClusterInfo" RPC request and reads the
 * reply.
 */
bool
S9sRpcClient::getCluster(
        int clusterId)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/0/clusters/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "getAllClusterInfo";
    request["with_hosts"] = true;
    request["cluster_id"] = clusterId;
    request["user"]           = options->userName();
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());
    //printf("-->\n%s\n", STR(reply().toString()));
    //exit (0);
    return retval;
}


/**
 * The method that sends the "getAllClusterInfo" RPC request and reads the
 * reply.
 */
bool
S9sRpcClient::getClusters()
{
    S9sOptions    *options = S9sOptions::instance();
    S9sString      uri = "/0/clusters/";
    S9sVariantMap  request;
    bool           retval;

    request["operation"]  = "getAllClusterInfo";
    request["with_hosts"] = true;
    request["user"]           = options->userName();
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;
}

/**
 * \param clusterId The cluster where the request will be sent.
 * \param hosts The list of hosts to change (currently only one host is
 *   supported).
 * \param properties The names and values of the host properties to change.
 */
bool
S9sRpcClient::setHost(
        const int             clusterId,
        const S9sVariantList &hosts,
        const S9sVariantMap  &properties)
{
    S9sString      uri;
    S9sVariantMap  request;

    uri.sprintf("/%d/stat", clusterId);

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
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

        
    return executeRequest(uri, request.toString());
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
    S9sString      uri;
    S9sVariantMap  request;
    bool           retval;

    uri.sprintf("/%d/stat/", clusterId);

    request["operation"] = "getCpuPhysicalInfo";
    //request["including_hosts"] = "192.168.1.101;192.168.1.102;192.168.1.104";

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    S9S_DEBUG("uri     : %s", STR(uri));
    S9S_DEBUG("request : %s", STR(request.toString()));
    retval = executeRequest(uri, request.toString());
    S9S_DEBUG("retval  : %s", retval ? "true" : "false");
    S9S_DEBUG("error   : %s", STR(m_priv->m_errorString));

    return retval;
}

/**
 * \param clusterId the ID of the cluster for which the CPU information will be
 *   fetched.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
    {
        "busy": 0.0664384,
        "cpuid": 2,
        "cpumhz": 2661.57,
        "cpumodelname": "Intel(R) Xeon(R) CPU           X5650  @ 2.67GHz",
        "cpuphysicalid": 0,
        "cputemp": 0,
        "created": 1476084629,
        "hostid": 3,
        "idle": 0.930993,
        "interval": 59486,
        "iowait": 0.00256849,
        "loadavg1": 0.26,
        "loadavg15": 0.18,
        "loadavg5": 0.28,
        "sampleends": 1476084629,
        "samplekey": "CmonCpuStats-3-2",
        "steal": 0,
        "sys": 0.0438356,
        "uptime": 993.05,
        "user": 0.0226027
    }
 */
bool
S9sRpcClient::getCpuStats(
        const int clusterId)
{
    S9sString      uri;
    S9sVariantMap  request;
    bool           retval;

    uri.sprintf("/%d/stat/", clusterId);

    request["operation"] = "statByName";
    request["name"]      = "cpustat";

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());
    
    return retval;
}

bool
S9sRpcClient::getMemoryStats(
        const int clusterId)
{
    S9sString      uri;
    S9sVariantMap  request;
    bool           retval;

    uri.sprintf("/%d/stat/", clusterId);

    request["operation"] = "statByName";
    request["name"]      = "memorystat";

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());
    
    return retval;
}

/**
 * \param clusterId the ID of the cluster for which the process information will
 *   be fetched.
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A method to get the list of the running processes from all nodes of one
 * particular cluster.
 */
bool 
S9sRpcClient::getRunningProcesses(
        const int clusterId)
{
    S9sString      uri;
    S9sVariantMap  request;
    bool           retval;

    uri.sprintf("/%d/proc/", clusterId);

    request["operation"] = "getRunningProcesses";
    //request["including_hosts"] = "192.168.1.101;192.168.1.102;192.168.1.104";

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    S9S_DEBUG("uri     : %s", STR(uri));
    S9S_DEBUG("request : %s", STR(request.toString()));
    retval = executeRequest(uri, request.toString());
    S9S_DEBUG("retval  : %s", retval ? "true" : "false");
    S9S_DEBUG("error   : %s", STR(m_priv->m_errorString));

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
        const int clusterId)
{
    S9sString      uri;
    S9sVariantMap  request;
    bool           retval;

    uri.sprintf("/%d/job/", clusterId);

    request["operation"] = "getJobInstances";

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

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
        const int clusterId,
        const int jobId)
{
    S9sString      uri;
    S9sVariantMap  request;
    bool           retval;

    uri.sprintf("/%d/job/", clusterId);

    request["operation"] = "getJobInstance";
    request["job_id"]    = jobId;

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;
}

/**
 * \param clusterId the ID of the cluster that owns the job
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
        const int clusterId,
        const int jobId,
        const int limit,
        const int offset)
{
    S9sString      uri;
    S9sVariantMap  request;
    bool           retval;

    uri.sprintf("/%d/job/", clusterId);

    // Building the request.
    request["operation"]  = "getJobLog";
    request["job_id"]     = jobId;
    request["ascending"]  = true;
    if (limit != 0)
        request["limit"]  = limit;

    if (offset != 0)
        request["offset"] = offset;

    if (!m_priv->m_token.empty())
        request["token"]  = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;

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
    S9sString      uri;
    bool           retval;

    uri.sprintf("/%d/job/", clusterId);

    jobSpec["command"]   = "rolling_restart";

    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Rolling Restart";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();

    request["operation"] = "createJobInstance";
    request["job"]       = job;
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;
    
    retval = executeRequest(uri, request.toString());

    return retval;
}

#if 0
{
    "command": "create_cluster",
    "job_data": 
    {
        "api_id": "1",
        "cluster_type": "group_replication",
        "create_local_repository": false,
        "data_center": 0,
        "disable_firewall": true,
        "disable_selinux": true,
        "enable_mysql_uninstall": true,
        "generate_token": true,
        "install_software": true,
        "mysql_cnf_template": "my.cnf.repl57",
        "mysql_datadir": "/var/lib/mysql",
        "mysql_hostnames": [ "10.10.10.15" ],
        "mysql_password": "password",
        "mysql_port": "3306",
        "mysql_version": "5.7",
        "ssh_keyfile": "/home/sergey/.ssh/id_rsa",
        "ssh_port": "22",
        "ssh_user": "sergey",
        "sudo_password": "",
        "type": "mysql",
        "use_internal_repos": false,
        "user_id": 1,
        "vendor": "oracle"
    }
}
#endif

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
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
    S9sVariantList  hostNames;
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri;
    bool            retval;
    
    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Galera cluster.");
        return false;
    }
    
    uri = "/0/job/";
    for (uint idx = 0; idx < hosts.size(); ++idx)
    {
        if (hosts[idx].isNode())
            hostNames << hosts[idx].toNode().hostName();
        else
            hostNames << hosts[idx];
    }

    // The job_data describing the cluster.
    jobData["cluster_type"]    = "galera";
    jobData["mysql_hostnames"] = hostNames;
    jobData["vendor"]          = vendor;
    jobData["mysql_version"]   = mySqlVersion;
    jobData["enable_mysql_uninstall"] = uninstall;
    jobData["ssh_user"]        = osUserName;
    //jobData["repl_user"]        = options->dbAdminUserName();
    jobData["mysql_password"]  = options->dbAdminPassword();
    
    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();
    
    if (!options->osKeyFile().empty())
        jobData["ssh_key"]     = options->osKeyFile();

    // The jobspec describing the command.
    jobSpec["command"]  = "create_cluster";
    jobSpec["job_data"] = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Create Galera Cluster";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());
    
    return retval;
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
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
    S9sVariantList  hostNames;
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri = "/0/job/";
    bool            retval;
    
    if (hosts.size() < 1u)
    {
        PRINT_ERROR("Missing node list while creating Galera cluster.");
        return false;
    }
    
    uri = "/0/job/";
    for (uint idx = 0; idx < hosts.size(); ++idx)
    {
        if (hosts[idx].isNode())
            hostNames << hosts[idx].toNode().hostName();
        else
            hostNames << hosts[idx];
    }

    // The job_data describing the cluster.
    jobData["cluster_type"]     = "replication";
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
    jobSpec["command"]  = "create_cluster";
    jobSpec["job_data"] = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Create MySQL Replication Cluster";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();
    //job["api_id"]        = -1;

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;
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
    S9sString       uri = "/0/job/";
    bool            retval;
    
    uri = "/0/job/";
    
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
        jobData["ssh_key"]      = options->osKeyFile();

    // The jobspec describing the command.
    jobSpec["command"]  = "create_cluster";
    jobSpec["job_data"] = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Create NDB Cluster";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();
    //job["api_id"]        = -1;

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;
}

/**
 * \param hosts the hosts that will be the member of the cluster (variant list
 *   with S9sNode elements).
 *
 */
bool
S9sRpcClient::createPostgreSql(
        const S9sVariantList &hosts,
        const S9sString      &osUserName,
        bool                  uninstall)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  hostNames;
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri;
    bool            retval;

    if (hosts.size() != 1u)
    {
        PRINT_ERROR("PostgreSQL can only be created with one server in it.");
        return false;
    }

    uri = "/0/job/";
    for (uint idx = 0; idx < hosts.size(); ++idx)
    {
        if (hosts[idx].isNode())
            hostNames << hosts[idx].toNode().hostName();
        else
            hostNames << hosts[idx];
    }

    // The job_data describing the cluster.
    jobData["cluster_type"]     = "postgresql_single";
    jobData["type"]             = "postgresql";
    jobData["hostname"]         = hostNames[0];
    jobData["enable_uninstall"] = uninstall;
    jobData["ssh_user"]         = osUserName;
    jobData["postgre_user"]     = options->dbAdminUserName();
    jobData["postgre_password"] = options->dbAdminPassword();

    if (!options->clusterName().empty())
        jobData["cluster_name"] = options->clusterName();
    
    // The jobspec describing the command.
    jobSpec["command"]   = "setup_server";
    jobSpec["job_data"]  = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Setup PostgreSQL Server";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    S9S_WARNING("-> %s", STR(request.toString()));
    retval = executeRequest(uri, request.toString());
    
    return retval;
}


/**
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
    S9sString      uri;
    bool           retval;

    if (hosts.size() != 1u)
    {
        PRINT_ERROR("addnode is currently implemented only for one node.");
        return false;
    }
    
    uri.sprintf("/%d/job/", clusterId);

    // The job_data describing the cluster.
    if (hosts[0].isNode())
        jobData["hostname"] = hosts[0].toNode().hostName();
    else 
        jobData["hostname"] = hosts[0].toString();

    jobData["install_software"] = true;
    jobData["disable_firewall"] = true;
    jobData["disable_selinux"]  = true;
   
    // The jobspec describing the command.
    jobSpec["command"]  = "addnode";
    jobSpec["job_data"] = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Add Node to Cluster";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();
    //job["api_id"]        = -1;

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;
}

/**
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
    S9sString      uri;
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

    uri.sprintf("/%d/job/", clusterId);
    
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
    jobSpec["command"]  = "haproxy";
    jobSpec["job_data"] = jobData;
    
    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Add HaProxy to Cluster";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();
    //job["api_id"]        = -1;

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;
}

/**
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
    S9sString      uri;
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

    uri.sprintf("/%d/job/", clusterId);
    
    // The job_data describing the cluster.
    jobData["action"]   = "setupProxySql";
    jobData["hostname"] = proxyNodes[0].toNode().hostName();
    
    // The jobspec describing the command.
    jobSpec["command"]  = "proxysql";
    jobSpec["job_data"] = jobData;
    
    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Add ProxySQL to Cluster";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();
    //job["api_id"]        = -1;

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;
}

/**
 * \returns true if the request sent and a return is received (even if the reply
 *   is an error message).
 *
 * A function to create a job that will add a proxysql host to the cluster.
 */
bool
S9sRpcClient::addMaxScale(
        const int             clusterId,
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri;
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

    uri.sprintf("/%d/job/", clusterId);

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
    jobSpec["command"]  = "maxscale";
    jobSpec["job_data"] = jobData;
    
    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Add MaxScale to Cluster";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();
    //job["api_id"]        = -1;

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

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
S9sRpcClient::removeNode(
        const int             clusterId,
        const S9sVariantList &hosts)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      hostName, title;
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri;
    bool           retval;

    if (hosts.size() != 1u)
    {
        PRINT_ERROR("removenode is currently implemented only for one node.");
        return false;
    }
    
    uri.sprintf("/%d/job/", clusterId);
    hostName = hosts[0].toNode().hostName();
    title.sprintf("Remove '%s' from the Cluster", STR(hostName));

    // The job_data describing the cluster.
    jobData["host"]             = hostName;
    //jobData["port"]             =
   
    // The jobspec describing the command.
    jobSpec["command"]  = "removenode";
    jobSpec["job_data"] = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = title;
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();
    //job["api_id"]        = -1;

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;
}

/**
 * \param clusterId The ID of the cluster.
 * \returns true if the request was sent and the reply was received (even if the
 *   reply is an error notification).
 *
 * This function will stop the cluster by creating a "stop_cluster" job.
 */
bool
S9sRpcClient::stopCluster(
        const int             clusterId)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      title;
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri;
    bool           retval;
    
    uri.sprintf("/%d/job/", clusterId);
    title = "Stopping Cluster";

    // The job_data describing the cluster.
    jobData["force"]               = false;
    jobData["stop_timeout"]        = 1800;
    jobData["maintenance_minutes"] = 0;
    jobData["reason"]              = "Cluster is stopped.";
    
    // The jobspec describing the command.
    jobSpec["command"]   = "stop_cluster";
    jobSpec["job_data"]  = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = title;
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();
    
    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

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
S9sRpcClient::dropCluster(
        const int             clusterId)
{
    S9sOptions    *options   = S9sOptions::instance();
    S9sString      title;
    S9sVariantMap  request;
    S9sVariantMap  job, jobData, jobSpec;
    S9sString      uri;
    bool           retval;
    
    uri.sprintf("/%d/job/", clusterId);
    title = "Remove Cluster";

    // The job_data describing the cluster.
    jobData["clusterid"]       = clusterId;
    
    // The jobspec describing the command.
    jobSpec["command"]  = "remove_node";
    jobSpec["job_data"] = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = title;
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();
    
    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;
}

bool
S9sRpcClient::createBackup(
        const int             clusterId,
        const S9sVariantList &hosts)
{
    S9sOptions     *options      = S9sOptions::instance();
    S9sString       backupMethod = options->backupMethod();
    S9sString       backupDir    = options->backupDir();
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri;
    bool            retval;

    uri.sprintf("/%d/job/", clusterId);
    
    if (hosts.size() != 1u)
    {
        PRINT_ERROR("To create a new backup one node must be specified.");

        return false;
    }

    // The job_data describing how the backup will be created.
    jobData["hostname"]         = hosts[0].toNode().hostName();
    jobData["backupdir"]        = "/tmp";
    if (!backupMethod.empty())
        jobData["backup_method"]    = backupMethod;

    // The jobspec describing the command.
    jobSpec["command"]   = "backup";
    jobSpec["job_data"]  = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Create Backup";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());
    
    return retval;
}

bool
S9sRpcClient::restoreBackup(
        const int             clusterId,
        const int             backupId)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sString       backupMethod = options->backupMethod();
    S9sVariantMap   request;
    S9sVariantMap   job, jobData, jobSpec;
    S9sString       uri;
    bool            retval;

    uri.sprintf("/%d/job/", clusterId);
    
    // The job_data describing how the backup will be created.
    jobData["backupid"]        = backupId;
    jobData["bootstrap"]        = true;

    // The jobspec describing the command.
    jobSpec["command"]   = "restore_backup";
    jobSpec["job_data"]  = jobData;

    // The job instance describing how the job will be executed.
    job["class_name"]    = "CmonJobInstance";
    job["title"]         = "Restore Backup";
    job["job_spec"]      = jobSpec;
    job["user_name"]     = options->userName();

    // The request describing we want to register a job instance.
    request["operation"] = "createJobInstance";
    request["job"]       = job;
    
    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());
    
    return retval;
}

bool
S9sRpcClient::getBackups(
        const int clusterId)
{
    S9sString      uri;
    S9sVariantMap  request;
    bool           retval;

    uri.sprintf("/%d/backup/", clusterId);

    request["operation"] = "listBackups";

    if (!m_priv->m_token.empty())
        request["token"] = m_priv->m_token;

    retval = executeRequest(uri, request.toString());

    return retval;
}


        
/**
 * \param uri the file path part of the URL where we send the request
 * \param payload the JSON request string
 * \returns true if everything is ok, false on error.
 */
bool
S9sRpcClient::executeRequest(
        const S9sString &uri,
        const S9sString &payload)
{
    S9sOptions  *options = S9sOptions::instance();    
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
     *
     */
    if (options->isJsonRequested() && options->isVerbose())
        printf("Preparing to send request: \n%s\n", STR(payload));

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
        // priv shall do this on failure
        m_priv->m_errorString.sprintf("Error reading socket: %m");
        return false;
    }

    if (!m_priv->m_reply.parse(STR(m_priv->m_jsonReply)))
    {
        PRINT_ERROR("Error parsing JSON reply.");
        m_priv->m_errorString.sprintf("Error parsing JSON reply.");
        return false;
    }

    //printf("-> \n%s\n", STR(m_priv->m_reply.toString()));
    return true;
}

