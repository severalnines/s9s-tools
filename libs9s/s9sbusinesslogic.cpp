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

#include "S9sRpcReply"
#include "S9sOptions"
#include "S9sNode"
#include "S9sDateTime"

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
    S9sString    controller = options->controller();
    int          port       = options->controllerPort();
    S9sString    token      = options->rpcToken();
    int          clusterId  = options->clusterId();
    S9sRpcClient client(controller, port, token);

    S9S_DEBUG("");
    if (options->isClusterOperation())
    {
        if (options->isListRequested())
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
    S9sVariantList hostNames;
    S9sRpcReply    reply;
    bool           success;

    hostNames = options->nodes();
    if (hostNames.empty())
    {
        options->printError(
                "Node list is empty while adding node.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    /*
     * Running the request on the controller.
     */
    success = client.addNode(clusterId, hostNames);
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
S9sBusinessLogic::executeTop(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    int          clusterId = options->clusterId();
    S9sRpcReply reply;

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
    // A small test to get the memory info.
    //
    client.getMemoryStats(options->clusterId());
    reply = client.reply();
    printf("%s\n", STR(reply.toString()));
    reply.printMemoryStatLine1();
    exit(0);
    #endif

    for (;;)
    {
        S9sDateTime date = S9sDateTime::currentDateTime();
        S9sString   dateString = date.toString(S9sDateTime::LongTimeFormat);
        printf("\033[0;0H");

        //
        // The date.
        //
        printf("s-s - %s \n", STR(dateString));

        //
        // Summary of CPU usage.
        //
        client.getCpuStats(clusterId);
        reply = client.reply();
        reply.printCpuStatLine1();
   
        //
        // The memory summary.
        //
        client.getMemoryStats(options->clusterId());
        reply = client.reply();
        reply.printMemoryStatLine1();
        reply.printMemoryStatLine2();
        printf("\n");

        //
        // List of processes.
        //
        client.getRunningProcesses(clusterId);
        reply = client.reply();
        reply.printProcessList(options->terminalHeight() - 5);

        sleep(3);
    }
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
    S9sRpcReply reply;
    int         clusterId = options->clusterId();
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
    int         clusterId = options->clusterId();
    int         jobId     = options->jobId();
    bool        success;

    success = client.getJobLog(clusterId, jobId);
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
    } else {
        if (options->isJsonRequested())
            printf("%s\n", STR(reply.toString()));
        else
            PRINT_ERROR("%s", STR(client.errorString()));
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
    S9sOptions  *options         = S9sOptions::instance();
    bool         syntaxHighlight = options->useSyntaxHighlight();
    const char  *rotate[]        = { "/", "-", "\\", "|" };
    int          rotateCycle     = 0;
    S9sRpcReply  reply;
    bool         success, finished;
    S9sString    progressLine;
    bool         titlePrinted = false;
    int          nFailures = 0;

    if (syntaxHighlight)
        printf("\033[?25l"); 

    for (;;)
    {
        /*
         * Getting the job instance from the controller.
         */
        success = client.getJobInstance(clusterId, jobId);
        if (success)
        {
            reply = client.reply();
            success = reply.isOk();
        }
        
        if (!success)
        {
            ++nFailures;
            if (nFailures > 60)
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

    for (;;)
    {
        success = client.getJobLog(clusterId, jobId, 300, nLogsPrinted);
        if (!success)
            continue;

        reply    = client.reply();
        success  = reply.isOk();
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
