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
#include "s9sbusinesslogic.h"

#include "S9sStringList"
#include "S9sRpcReply"
#include "S9sOptions"
#include "S9sNode"
#include "S9sDateTime"
#include "S9sTopUi"
#include "S9sFile"
#include "S9sRsaKey"

#include <stdio.h>
#include <unistd.h>

//#define DEBUG
#include "s9sdebug.h"

/**
 * This method will execute whatever is requested by the user in the command
 * line.
 */
void 
S9sBusinessLogic::execute()
{
    S9sOptions  *options    = S9sOptions::instance();
    S9sString    controller = options->controllerHostName();
    int          port       = options->controllerPort();
    S9sString    token      = options->rpcToken();
    int          clusterId  = options->clusterId();
    bool         useTls     = options->useTls();
    S9sRpcClient client(controller, port, token, useTls);

    /*
     * Here is a fucked up version and a version that I try to clean up.
     */
    //if (!options->isUserOperation() || !options->rpcToken().empty())
    if (options->isUserOperation() && options->isCreateRequested())
    {
        PRINT_VERBOSE("No authentication required");
        // No authentication required.
    } else {
        PRINT_VERBOSE("Authenticating.");
        S9sString userName = options->userName();
        S9sString keyPath  = options->privateKeyPath();

        if (userName.empty())
        {
            PRINT_ERROR("No user name not set.");
            options->setExitStatus(S9sOptions::BadOptions);
            return;
        }

        if (keyPath.empty())
        {
            PRINT_ERROR("No key file name not set.");
            options->setExitStatus(S9sOptions::BadOptions);
            return;
        }

        if (!client.authenticate())
        {

            if (options->isJsonRequested())
            {
                printf("%s\n", STR(client.reply().toString()));
            } else {
                S9sString errorString = client.errorString();

                if (errorString.empty())
                    errorString = client.reply().errorString();

                if (errorString.empty())
                    errorString = "Access denied.";

                PRINT_ERROR("Authentication failed: %s", STR(errorString));
            }

            // continuing, server replies a nice error
            options->setExitStatus(S9sOptions::AccessDenied);
            return;
        }
    }

    S9S_DEBUG("");
    if (options->isClusterOperation())
    {
        if (options->isPingRequested())
        {
            executePing(client);
        } else if (options->isListRequested())
        {
            executeClusterList(client);
        } else if (options->isCreateRequested())
        {
            executeClusterCreate(client);
        } else if (options->isRollingRestartRequested())
        {
            executeRollingRestart(client);
        } else if (options->isAddNodeRequested())
        {
            executeAddNode(client);
        } else if (options->isRemoveNodeRequested())
        {
            executeRemoveNode(client);
        } else if (options->isStopRequested())
        {
            executeStopCluster(client);
        } else if (options->isStartRequested())
        {
            executeStartCluster(client);
        } else if (options->isDropRequested())
        {
            executeDropCluster(client);
        } else {
            PRINT_ERROR("Operation is not specified.");
        }
    } else if (options->isNodeOperation())
    {
        if (options->isListRequested())
        {
            executeNodeList(client);
        } else if (options->isSetRequested())
        {
            executeNodeSet(client);
        } else {
            PRINT_ERROR("Operation is not specified.");
        }
    } else if (options->isJobOperation())
    {
        if (options->isListRequested())
        {
            executeJobList(client);
        } else if (options->isLogRequested())
        {
            executeJobLog(client);
        } else if (options->isWaitRequested())
        {
            waitForJob(clusterId, options->jobId(), client);
        } else {
            PRINT_ERROR("Unknown job operation.");
        }
    } else if (options->isBackupOperation())
    {
        if (options->isListRequested())
        {
            executeBackupList(client);
        } else if (options->isCreateRequested())
        {
            executeCreateBackup(client);
        } else if (options->isRestoreRequested())
        {
            executeRestoreBackup(client);
        } else {
            PRINT_ERROR("Unknown backup operation.");
        }
    } else if (options->isProcessOperation())
    {
        if (options->isListRequested())
        {
            executeProcessList(client);
        } else if (options->isTopRequested())
        {
            executeTop(client);
        } else {
            PRINT_ERROR("Unknown process operation.");
        }
    } else if (options->isUserOperation())
    {
        if (options->isListRequested() || options->isWhoAmIRequested())
        {
            executeUserList(client);
        } else {
            executeUser(client);
        }
    } else if (options->isMaintenanceOperation())
    {
        if (options->isListRequested())
        {
            executeMaintenanceList(client);
        } else if (options->isCreateRequested())
        {
            executeMaintenanceCreate(client);
        } else if (options->isDeleteRequested())
        {
            executeMaintenanceDelete(client);
        } else {
            PRINT_ERROR("Unknown maintenance operation.");
        }
    } else {
        PRINT_ERROR("Unknown operation.");
    }
}

/**
 * \param client A client for the communication.
 * \param jobId The ID of the job that we wait for.
 * \param client A client for the communication.
 *
 * This method waits for the given job to be finished (or failed, or aborted)
 * and will provide feedback in the form the user requested in the command 
 * line (printing job messages or a progress bar).
 */
void 
S9sBusinessLogic::waitForJob(
        const int       clusterId,
        const int       jobId, 
        S9sRpcClient   &client)
{
    S9sOptions  *options = S9sOptions::instance();
    
    S9S_DEBUG("");
    if (options->isLogRequested())
    {
        waitForJobWithLog(clusterId, jobId, client);
    } else {
        waitForJobWithProgress(clusterId, jobId, client);
    }
}

/**
 * \param client A client for the communication.
 *
 * Execute the "cluster --create" requests.
 */
void
S9sBusinessLogic::executeClusterCreate(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
        
    if (options->clusterType() == "")
    {
        PRINT_ERROR(
                 "Cluster type is not set.\n"
                 "Use the --cluster-type command line option to set it.");

        options->setExitStatus(S9sOptions::BadOptions);
    } else if (options->clusterType() == "galera")
    {
        doExecuteCreateCluster(client);
    } else if (options->clusterType() == "mysqlreplication")
    {
        doExecuteCreateCluster(client);
    } else if (options->clusterType() == "ndb")
    {
        doExecuteCreateCluster(client);
    } else if (options->clusterType() == "postgresql")
    {
        doExecuteCreateCluster(client);
    } else {
        PRINT_ERROR(
                "Cluster type '%s' is not supported.",
                STR(options->clusterType()));

        options->setExitStatus(S9sOptions::BadOptions);
    }
}

/**
 * \param client A client for the communication.
 *
 * This method will register a new "addNode" job on the controller using the
 * help of the S9sRpcClint class.
 */
void
S9sBusinessLogic::executeAddNode(
        S9sRpcClient &client)
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
        return;
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

        return;
    } else if (hasHaproxy && hasMaxScale)
    {
        PRINT_ERROR(
                "It is not possible to add a HaProxy and a MaxScale node "
                "in one call.");

        return;
    } else if (hasProxySql && hasMaxScale)
    {
        PRINT_ERROR(
                "It is not possible to add a ProxySql and a MaxScale node "
                "in one call.");

        return;
    } else if (hasProxySql)
    {
        success = client.addProxySql(clusterId, hosts);
    } else if (hasHaproxy)
    {
        success = client.addHaProxy(clusterId, hosts);
    } else if (hasMaxScale)
    {
        success = client.addMaxScale(clusterId, hosts);
    } else {
        success = client.addNode(clusterId, hosts);
    }

    if (success)
    {
        jobRegistered(client, clusterId);
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 *
 * This method will register a new "removeNode" job on the controller using the
 * help of the S9sRpcClint class.
 */
void
S9sBusinessLogic::executeRemoveNode(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sVariantList hostNames;
    S9sRpcReply    reply;
    bool           success;

    hostNames = options->nodes();
    if (hostNames.empty())
    {
        options->printError(
                "Node list is empty while removing node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    /*
     * Running the request on the controller.
     */
    success = client.removeNode(clusterId, hostNames);
    if (success)
    {
        jobRegistered(client, clusterId);
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 *
 * This method will register a new "stop_cluster" job on the controller 
 * using the help of the S9sRpcClint class.
 */
void
S9sBusinessLogic::executeStopCluster(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sRpcReply    reply;
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.stopCluster(clusterId);
    if (success)
    {
        jobRegistered(client, clusterId);
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void
S9sBusinessLogic::executeStartCluster(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sRpcReply    reply;
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.startCluster(clusterId);
    if (success)
    {
        jobRegistered(client, clusterId);
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 *
 * This method will register a new "remove_cluster" job on the controller 
 * using the help of the S9sRpcClint class.
 */
void
S9sBusinessLogic::executeDropCluster(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sRpcReply    reply;
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.dropCluster(clusterId);
    if (success)
    {
        jobRegistered(client, clusterId);
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void
S9sBusinessLogic::executePing(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    bool         continuous = options->isWaitRequested();
    S9sRpcReply  reply;
    bool         success;

again:
    success = client.ping();
    if (success)
    {
        reply = client.reply();

        success = reply.isOk();
        if (success)
        {
            // well, nothing now
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                reply.printPing();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));

            options->setExitStatus(S9sOptions::Failed);
        }
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
            
        options->setExitStatus(S9sOptions::Failed);
    }

    if (continuous)
    {
        usleep(500000);
        goto again;
    }
}

/**
 * \param client A client for the communication.
 *
 * Execute the "cluster --list" operation.
 */
void
S9sBusinessLogic::executeClusterList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;

    success = client.getClusters();
    if (success)
    {
        reply = client.reply();

        success = reply.isOk();
        if (success)
        {
            reply.printClusterList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}
 
/**
 * \param client A client for the communication.
 *
 */
void 
S9sBusinessLogic::executeNodeList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;


    success = client.getClusters();
    if (success)
    {
        reply = client.reply();

        success = reply.isOk();
        if (success)
        {
            reply.printNodeList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 *
 */
void 
S9sBusinessLogic::executeNodeSet(
        S9sRpcClient &client)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sRpcReply     reply;
    S9sVariantList  hostNames;
    S9sVariantMap   properties;
    bool            success;
    int             clusterId = options->clusterId();

    hostNames = options->nodes();
    if (hostNames.empty())
    {
        options->printError(
                "Node list is empty while setting node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }
    
    properties = options->propertiesOption();
    if (properties.empty())
    {
        options->printError(
                "Properties not provided while setting node.\n"
                "Use the --properties command line option to provide"
                " properties."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    success = client.setHost(clusterId, hostNames, properties);
    if (options->isJsonRequested())
    {
        reply = client.reply();
        printf("%s\n", STR(reply.toString()));
    } else {
        if (success)
            printf("OK\n");
    }
}

void
S9sBusinessLogic::executeCreateBackup(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    int            clusterId = options->clusterId();
    S9sRpcReply    reply;
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.createBackup(clusterId, options->nodes());
    if (success)
    {
        jobRegistered(client, clusterId);
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void
S9sBusinessLogic::executeRestoreBackup(
        S9sRpcClient &client)
{
    S9sOptions    *options   = S9sOptions::instance();
    int            clusterId = options->clusterId();
    int            backupId  = options->backupId();
    S9sRpcReply    reply;
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.restoreBackup(clusterId, backupId);
    if (success)
    {
        jobRegistered(client, clusterId);
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * This method provides a continuous display of one specific cluster that is
 * similar to the "top" utility.
 */
void 
S9sBusinessLogic::executeTop(
        S9sRpcClient &client)
{
    #if 0
    //
    // A small test to get the cpu info.
    //
    client.getCpuInfo(options->clusterId());
    reply = client.reply();
    printf("%s\n", STR(reply.toString()));
    #endif
    
    #if 0
    //
    // A small test to get the cpu info.
    //
    client.getCpuStats(options->clusterId());
    reply = client.reply();
    printf("%s\n", STR(reply.toString()));
    exit(0);
    #endif
    
    #if 0
    //
    // A small test to get the memory info.
    //
    client.getMemoryStats(options->clusterId());
    reply = client.reply();
    printf("%s\n", STR(reply.toString()));
    reply.printMemoryStatLine1();
    exit(0);
    #endif

    S9sTopUi ui;

    ui.executeTop(client);
}

/**
 * \param client A client for the communication.
 *
 * Executes the --list operation on the processes thus providing a list of
 * running processes.
 */
void 
S9sBusinessLogic::executeProcessList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    int         clusterId = options->clusterId();
    S9sRpcReply reply;
    bool        success;

    success = client.getRunningProcesses(clusterId);
    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            if (options->isJsonRequested())
                printf("\n%s\n", STR(reply.toString()));
            else
                reply.printProcessList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void 
S9sBusinessLogic::executeBackupList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    int         clusterId = options->clusterId();
    S9sRpcReply reply;
    bool        success;

    success = client.getBackups(clusterId);
    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            if (options->isJsonRequested())
                printf("\n%s\n", STR(reply.toString()));
            else
                reply.printBackupList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * This function will execute the listing of the users that can be requested by
 * the --list and --whoami command line options.
 */
void 
S9sBusinessLogic::executeUserList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;

    success = client.getUsers();
    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            if (options->isJsonRequested())
                printf("\n%s\n", STR(reply.toString()));
            else
                reply.printUserList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 *
 * Executes the --list operation on the jobs thus providing a list of jobs.
 */
void 
S9sBusinessLogic::executeJobList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    int         clusterId = options->clusterId();
    bool        success;

    success = client.getJobInstances(clusterId);
    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            reply.printJobList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    } 
}

/**
 * \param client A client for the communication.
 * 
 * This method will get the job messages from the controller and print them to
 * the standard output.
 */
void 
S9sBusinessLogic::executeJobLog(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    int         jobId     = options->jobId();
    bool        success;

    success = client.getJobLog(jobId);
    if (success)
    {
        reply = client.reply();
        
        success = reply.isOk();
        if (success)
        {
            reply.printJobLog();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }

    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
}
}

/**
 * \param client A client for the communication.
 *
 * This method will start a rolling-restart job on the controller. 
 */
void 
S9sBusinessLogic::executeRollingRestart(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    int         clusterId = options->clusterId();
    S9sRpcReply reply;
    bool        success;

    success = client.rollingRestart(clusterId);
    if (success)
    {
        jobRegistered(client, clusterId);
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client A client for the communication.
 *
 * This private function will execute the create cluster request that will
 * register a job to create a new cluster.
 */
void
S9sBusinessLogic::doExecuteCreateCluster(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantList hosts;
    S9sString      osUserName;
    S9sString      vendor;
    S9sString      mySqlVersion;
    bool           uninstall = true;
    S9sRpcReply    reply;
    bool           success;

    hosts = options->nodes();
    if (hosts.empty())
    {
        options->printError(
            "Node list is empty while creating cluster.\n"
            "Use the --nodes command line option to provide the node list."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    mySqlVersion = options->providerVersion("5.6");
    osUserName   = options->osUser();
    vendor       = options->vendor();

    if (vendor.empty() && options->clusterType() != "postgresql")
    {
        options->printError(
            "The vendor name is unknown while creating a galera cluster.\n"
            "Use the --vendor command line option to provide the vendor."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    if (mySqlVersion.empty())
    {
        options->printError(
            "The SQL server version is unknown while creating a cluster.\n"
            "Use the --provider-version command line option set it."
            );

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    /*
     * Running the request on the controller.
     */
    if (options->clusterType() == "galera")
    {
        success = client.createGaleraCluster(
                hosts, osUserName, vendor, 
                mySqlVersion, uninstall);
    } else if (options->clusterType() == "mysqlreplication")
    {
        success = client.createMySqlReplication(
                hosts, osUserName, vendor, 
                mySqlVersion, uninstall);
    } else if (options->clusterType() == "postgresql")
    {
        success = client.createPostgreSql(
                hosts, osUserName, uninstall);
    } else if (options->clusterType() == "ndb" || 
            options->clusterType() == "ndbcluster")
    {
        S9sVariantList mySqlHosts, mgmdHosts, ndbdHosts;

        for (uint idx = 0u; idx < hosts.size(); ++idx)
        {
            S9sNode     node     = hosts[idx].toNode();
            S9sString   protocol = node.protocol().toLower();

            S9S_DEBUG("*** idx      : %d", idx);
            S9S_DEBUG("*** type     : %s", STR(hosts[idx].typeName()));
            S9S_DEBUG("*** node     : %s", STR(node.hostName()));
            S9S_DEBUG("*** protocol : %s", STR(protocol));
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
                return;
            }
        }

        success = client.createNdbCluster(
                mySqlHosts, mgmdHosts, ndbdHosts,
                osUserName, vendor, mySqlVersion, uninstall);
    } else {
        success = false;
    }

    if (success)
    {
        // FIXME: this method happens to know that the request sent to the 0
        // cluster, but this is not exactly robust like this.
        jobRegistered(client, 0);
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/**
 * \param client The client that just created the new job with the reply in it.
 * \param clusterId The ID of the cluster that executes the job.
 *
 * This method can be called when a new job is registered using the RPC client.
 * This function will print the necessary messages to inform the user about the
 * new job, wait for the job to ended if necessary, show the progress or the job
 * messages if the command line options or settings suggests that should be
 * done.
 */
void
S9sBusinessLogic::jobRegistered(
        S9sRpcClient &client,
        const int     clusterId)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRpcReply    reply;
    bool           success;

    reply = client.reply();
    success = reply.isOk();
    
    if (success)
    {
        if (options->isWaitRequested() || options->isLogRequested())
        {
            waitForJob(clusterId, reply.jobId(), client);
        } else {
            reply.printJobStarted();
        }
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(reply.errorString()));
    }
}

/**
 * \param jobId The ID of the job to monitor with a progress bar.
 * \param client The client for the communication.
 *
 * This method can be used to wait until a job is finished and print a progress
 * report in the meantime.
 */
void 
S9sBusinessLogic::waitForJobWithProgress(
        const int     clusterId,
        const int     jobId, 
        S9sRpcClient &client)
{
    S9sOptions    *options         = S9sOptions::instance();
    bool           syntaxHighlight = options->useSyntaxHighlight();
    const char    *rotate[]        = { "/", "-", "\\", "|" };
    int            rotateCycle     = 0;
    S9sRpcReply    reply;
    bool           success, finished;
    S9sString      progressLine;
    bool           titlePrinted = false;
    int            nFailures = 0;
    int            nAuthentications = 0;

    if (syntaxHighlight)
        printf("\033[?25l"); 

    for (;;)
    {
        /*
         * Getting the job instance from the controller.
         */
        success = client.getJobInstance(jobId);
        if (success)
        {
            reply     = client.reply();
            success   = reply.isOk();

            if (reply.isAuthRequired())
            {
                if (nAuthentications > 3)
                    break;

                success = client.authenticate();
                ++nAuthentications;
            } else {
                nAuthentications = 0;
            }
        }
        
        if (success)
        {
            nFailures = 0;
        } else {
            PRINT_ERROR("%s", STR(reply.errorString()));
            printf("\n\n%s\n", STR(reply.toString()));
            ++nFailures;
            if (nFailures > 3)
                break;

            continue;
        }

        nFailures = 0;

        /*
         * Printing the title if it is not yet printed.
         */
        if (!titlePrinted && !reply.jobTitle().empty())
        {
            const char *titleBegin = "";
            const char *titleEnd   = "";

            if (syntaxHighlight)
            {
                titleBegin = TERM_BOLD;
                titleEnd   = TERM_NORMAL;
            }

            printf("%s%s%s\n", 
                    titleBegin,
                    STR(reply.jobTitle()),
                    titleEnd);

            titlePrinted = true;
        }

        /*
         *
         */
        finished = reply.progressLine(progressLine, syntaxHighlight);
        printf("%s %s\033[K\r", rotate[rotateCycle], STR(progressLine));

        if (reply.isJobFailed())
            options->setExitStatus(S9sOptions::JobFailed);

        //printf("%s", STR(reply.toString()));

        fflush(stdout);
        sleep(1);

        ++rotateCycle;
        rotateCycle %= sizeof(rotate) / sizeof(void *);

        if (finished)
            break;
    }

    if (syntaxHighlight)
        printf("\033[?25h");

    printf("\n");
}

/**
 * \param clusterId The ID of the cluster that executes the job.
 * \param jobId The ID of the job to monitor.
 * \param client The client for the communication.
 *
 * This function will wait for the job to be finished and will continuously
 * print the job messages.
 */
void 
S9sBusinessLogic::waitForJobWithLog(
        const int     clusterId,
        const int     jobId, 
        S9sRpcClient &client)
{
    S9sOptions    *options         = S9sOptions::instance();
    S9sVariantMap  job;
    S9sRpcReply    reply;
    bool           success, finished;
    int            nLogsPrinted = 0;
    int            nEntries;
    int            nFailures = 0;
    int            nAuthentications = 0;

    for (;;)
    {
        success = client.getJobLog(jobId, 300, nLogsPrinted);
        if (success)
        {
            reply     = client.reply();
            success   = reply.isOk();

            if (reply.isAuthRequired())
            {
                if (nAuthentications > 3)
                    break;

                success = client.authenticate();
                ++nAuthentications;
            } else {
                nAuthentications = 0;
            }
        }

        if (success)
        { 
            nFailures = 0;
        } else {
            PRINT_ERROR("%s", STR(reply.errorString()));
            printf("\n\n%s\n", STR(reply.toString()));
            ++nFailures;
            if (nFailures > 3)
                break;

            continue;
        }

        nEntries = reply["messages"].toVariantList().size();
        if (nEntries > 0)
            reply.printJobLog();

        nLogsPrinted += nEntries;

        job = reply["job"].toVariantMap();
        if (job["status"] == "FAILED")
            options->setExitStatus(S9sOptions::JobFailed);

        finished = 
            job["status"] == "ABORTED"   ||
            job["status"] == "FINISHED"  ||
            job["status"] == "FAILED";
        
        fflush(stdout);
        if (finished)
            break;
        
        sleep(1);
    }

    printf("\n");
}

void
S9sBusinessLogic::executeUser(
        S9sRpcClient        &client)
{
    S9sString    errorString;
    S9sOptions  *options  = S9sOptions::instance();
    S9sString    userName = options->userName();
    S9sString    keyFilePath;
    S9sConfigFile config;

    if (userName.empty())
    {
        PRINT_ERROR(
                 "User name is not set.\n"
                 "Use the --cmon-user command line option to set it.");

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    /*
     * Now make sure that we save the specified username into the user config
     * file
     */
    config.setFileName("~/.s9s/s9s.conf");
    PRINT_VERBOSE(
            "Saving Cmon user '%s' into config file at %s.",
            STR(userName), 
            STR(config.fileName()));

    if (!config.parseSourceFile())
    {
        PRINT_ERROR("Couldn't parse %s: %s",
                STR(config.fileName()), STR(config.errorString()));
        return;
    }

    if (config.hasVariable("global", "cmon_user"))
    {
        config.changeVariable("global", "cmon_user", userName);
    } else if (config.hasVariable("", "cmon_user"))
    {
        config.changeVariable("cmon_user", userName);
    } else {
        config.addVariable("global", "cmon_user", userName);
    }

    if (!config.save(errorString))
    {
        PRINT_ERROR(
                "Could not update user configuration file: %s", 
                STR(errorString));
        
        return;
    }

    /*
     *
     */
    S9sRsaKey rsaKey;

    keyFilePath = options->privateKeyPath();
    if (options->isGenerateKeyRequested())
    {
        // check if key exists and is valid, otherwise generate a new key-pair
        S9sRsaKey key;

        if (key.loadKeyFromFile(keyFilePath) && key.isValid())
        {
            PRINT_VERBOSE("Keyfile '%s' exists and valid.", STR(keyFilePath));
        } else {
            S9sString pubKeyPath;

            PRINT_VERBOSE("Generating an RSA key-pair (%s).", STR(keyFilePath));

            pubKeyPath = keyFilePath;
            pubKeyPath.replace(".key", "");
            pubKeyPath += ".pub";

            if (!key.generateKeyPair() ||
                !key.saveKeys(keyFilePath, pubKeyPath, errorString))
            {
                if (errorString.empty())
                    errorString = "RSA key generation failure.";

                PRINT_ERROR("Key generation failed: %s", STR(errorString));
            }
        }
    }

    if (!rsaKey.loadKeyFromFile(keyFilePath) || !rsaKey.isValid())
    {
        PRINT_ERROR("User keyfile couldn't be loaded: %s", STR(keyFilePath));
        options->setExitStatus(S9sOptions::JobFailed);
        return;
    }

    /*
     * Granting.
     */
    if (options->isCreateRequested())
    {
        bool          oneSucceed = false;
        int           exitCode = 0;
        S9sString     sshCommand;
        S9sStringList fifos;
        S9sString     pubKeyPath;

        // There must be a better way to determine this
        pubKeyPath = keyFilePath;
        pubKeyPath.replace(".key", "");
        pubKeyPath += ".pub";

        S9sFile pubKeyFile (pubKeyPath);
        S9sString pubKeyStr;
        if (!pubKeyFile.readTxtFile(pubKeyStr) || pubKeyStr.empty())
        {
            // Private key exists and valid, but
            PRINT_ERROR("Couldn't load public key (%s): %s",
                STR(pubKeyPath), STR(pubKeyFile.errorString()));

            options->setExitStatus(S9sOptions::JobFailed);
            return;
        }

        S9sString controller = options->controllerHostName();
        if (controller.empty())
            controller = "127.0.0.1";

        // this one is used by unit/functional tests
        if (S9sFile("/tmp/cmon_test/usermgmt.fifo").exists())
            fifos << "/tmp/cmon_test/usermgmt.fifo";

        // and in real cmon daemon
        fifos << "/var/lib/cmon/usermgmt.fifo";


        for (uint idx = 0; idx < fifos.size(); ++idx)
        {
            S9sVariantMap request;
            S9sString     escapedJson;
            S9sString     path = fifos.at(idx);
            S9sFile       file(path);

            // 
            // Building up a request for the user creation.
            //
            request["user_name"] = userName;
            request["pubkey"]    = pubKeyStr;
            
            if (!options->title().empty())
                request["title"] = options->title();
            
            if (!options->firstName().empty())
                request["first_name"] = options->firstName();
            
            if (!options->lastName().empty())
                request["last_name"] = options->lastName();
            
            if (!options->group().empty())
                request["group"] = options->group();

            if (options->createGroup())
                request["create_group"] = true;

            escapedJson = request.toString().escape();
            if (options->isJsonRequested())
                printf("Request: %s\n", STR(request.toString()));
            
            PRINT_VERBOSE("escapedJson: \n%s", STR(escapedJson));

            if (controller == "localhost" || controller == "127.0.0.1")
            {
                sshCommand.sprintf(
                    "echo \"%s\" "
                    "| sudo -n tee %s 2>/dev/null >/dev/null",
                    STR(escapedJson), STR(path));
            } else {
                sshCommand.sprintf(
                    "ssh -tt "
                    "-oUserKnownHostsFile=/dev/null "
                    "-oStrictHostKeyChecking=no "
                    "-oBatchMode=yes "
                    "-oPasswordAuthentication=no "
                    "-oConnectTimeout=30 "
                    " '%s' "
                    "'echo \"%s\" "
                    "| sudo -n tee %s 2>/dev/null >/dev/null'",
                    STR(controller),
                    STR(escapedJson),
                    STR(path));
            }

            PRINT_VERBOSE("Command: \n%s", STR(sshCommand));
            PRINT_VERBOSE(
                    "Tried to grant on %s:%s, exitCode=%d.",
                    STR(controller), STR(path), exitCode);

            exitCode    = ::system(STR(sshCommand));
            oneSucceed |= (exitCode == 0);

            if (exitCode != 0)
            {
                errorString.sprintf (
                    "SSH command exited with non-zero status: %d\n%s",
                    exitCode, STR(sshCommand));
            }
        }

        if (!oneSucceed)
        {
            PRINT_ERROR("%s", STR(errorString));
            options->setExitStatus(S9sOptions::JobFailed);

            return;
        } else {
            if (!options->isBatchRequested())
            {
                fprintf(stderr, 
                        "Grant user '%s' succeeded.\n", 
                        STR(userName));

                fflush(stderr);
            }
        }
    }
}

void
S9sBusinessLogic::executeMaintenanceCreate(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRpcReply    reply;
    bool           success;

    /*
     * Running the request on the controller.
     */
    if (options->hasClusterIdOption())
    {
        success = client.createMaintenance(
                options->clusterId(), options->start(), options->end(),
                options->reason());
    } else {
        success = client.createMaintenance(
                options->nodes(), options->start(), options->end(),
                options->reason());
    }

    reply   = client.reply();
    if (success)
    {
        if (options->isJsonRequested())
        {
            printf("%s\n", STR(reply.toString()));
        } else if (!options->isBatchRequested()) 
        {
            printf("%s\n", STR(reply.uuid()));
        }
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void
S9sBusinessLogic::executeMaintenanceDelete(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sRpcReply    reply;
    bool           success;

    /*
     * Running the request on the controller.
     */
    success = client.deleteMaintenance(options->uuid());
    reply   = client.reply();
    if (success)
    {
        if (options->isJsonRequested())
        {
            printf("%s\n", STR(reply.toString()));
        } else if (!options->isBatchRequested()) 
        {
            printf("Deleted.\n");
        }
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
    }
}

void 
S9sBusinessLogic::executeMaintenanceList(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    S9sRpcReply reply;
    bool        success;

    success = client.getMaintenance();
    if (success)
    {
        reply = client.reply();
        success = reply.isOk();
        if (success)
        {
            if (options->isJsonRequested())
                printf("\n%s\n", STR(reply.toString()));
            else
                reply.printMaintenanceList();
        } else {
            if (options->isJsonRequested())
                printf("%s\n", STR(reply.toString()));
            else
                PRINT_ERROR("%s", STR(reply.errorString()));
        }
    } else {
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}
