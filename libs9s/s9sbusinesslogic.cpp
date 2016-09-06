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

#include <stdio.h>
#include <unistd.h>

#define DEBUG
#include "s9sdebug.h"
        
void 
S9sBusinessLogic::execute()
{
    S9sOptions  *options = S9sOptions::instance();
    S9sString    controller = options->controller();
    int          port = options->controllerPort();
    S9sString    token = options->rpcToken();
    S9sRpcClient client(controller, port, token);

    if (options->isClusterOperation() && options->isListRequested())
    {
        executeClusterList(client);
    } else if (options->isClusterOperation() && options->isCreateRequested())
    {
        if (options->clusterType() == "")
        {
            options->printError(
                    "Cluster type is now set.\n"
                    "Use the --cluster-type command line option to set it.");

            options->setExitStatus(S9sOptions::BadOptions);
        } else if (options->clusterType() == "galera")
        {
            executeCreateGaleraCluster(client);
        } else {
            options->printError(
                    "Cluster type '%s' is not supported.",
                    STR(options->clusterType()));

            options->setExitStatus(S9sOptions::BadOptions);
        }
    } else if (options->isClusterOperation() && 
            options->isRollingRestartRequested())
    {
        executeRollingRestart(client);
    } else if (options->isNodeOperation() && options->isListRequested())
    {
        executeNodeList(client);
    } else if (options->isJobOperation() && options->isListRequested())
    {
        executeJobList(client);
    } else if (options->isJobOperation() && options->isLogRequested())
    {
        executeJobLog(client);
    } else if (options->isJobOperation() && options->isWaitRequested())
    {
        waitForJob(options->jobId(), client);
    }
}

void 
S9sBusinessLogic::waitForJob(
        const int     jobId, 
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    
    if (options->isLogRequested())
        waitForJobWithLog(jobId, client);
    else
        waitForJobWithProgess(jobId, client);
}

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
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}
        
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
        reply = client.reply();

        success = reply.isOk();
        if (success)
        {

            if (options->isWaitRequested() || options->isLogRequested())
            {
                waitForJob(reply.jobId(), client);
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
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}

/*
{
    "command": "create_cluster",
    "id": 0,
    "job_data": 
    {
        "enable_mysql_uninstall": true,
        "mysql_hostnames": [ "10.10.2.2", "10.10.2.3", "10.10.2.4" ],
        "mysql_version": "5.6",
        "ssh_user": "pipas",
        "vendor": "codership"
    }
}
 */
void
S9sBusinessLogic::executeCreateGaleraCluster(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    S9sVariantList hostNames;
    S9sString      osUserName;
    S9sString      vendor;
    S9sString      mySqlVersion;
    bool           uninstall = true;
    S9sRpcReply    reply;
    bool           success;

    hostNames = options->nodes();
    if (hostNames.empty())
    {
        options->printError(
                "Node list is empty while creating cluster.\n"
                "Use the --nodes command line option to provide the node list."
                );

        options->setExitStatus(S9sOptions::BadOptions);
        return;
    }

    mySqlVersion = options->providerVersion();
    if (mySqlVersion.empty())
        mySqlVersion = "5.6";

    osUserName = options->osUser();

    vendor = options->vendor();
    if (vendor.empty())
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
    success = client.createGaleraCluster(
            hostNames, osUserName, vendor, mySqlVersion, uninstall);
    if (success)
    {
        reply = client.reply();

        success = reply.isOk();
        if (success)
        {

            if (options->isWaitRequested() || options->isLogRequested())
            {
                waitForJob(reply.jobId(), client);
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
        PRINT_ERROR("%s", STR(client.errorString()));
    }
}
        
void 
S9sBusinessLogic::waitForJobWithProgess(
        const int     jobId, 
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    bool         syntaxHighlight = options->useSyntaxHighlight();
    int          clusterId = options->clusterId();
    const char  *rotate[] = { "/", "-", "\\", "|" };
    //const char  *rotate[] = { "˥", "˦", "˧", "˨", "˩" };
    //const char  *rotate[] = { "⇐", "⇖", "⇑", "⇗", "⇒", "⇘", "⇓", "⇙" };
    //const char   *rotate[] = { "◜ ", " ◝", " ◞", "◟ " };

    int          rotateCycle = 0;

    S9sRpcReply  reply;
    bool         success, finished;
    S9sString    progressLine;

    //printf("\n");
    printf("\033[?25l"); 
    for (;;)
    {
        success = client.getJobInstance(clusterId, jobId);
        if (success)
        {
            reply = client.reply();
            success = reply.isOk();
        }
        
        if (!success)
            continue;

        finished = reply.progressLine(progressLine, syntaxHighlight);
        printf("%s %s\033[K\r", rotate[rotateCycle], STR(progressLine));
        //printf("%s", STR(reply.toString()));
        fflush(stdout);
        sleep(1);

        ++rotateCycle;
        rotateCycle %= sizeof(rotate) / sizeof(void *);

        if (finished)
            break;
    }

    printf("\033[?25h");
    printf("\n");
}

void 
S9sBusinessLogic::waitForJobWithLog(
        const int     jobId, 
        S9sRpcClient &client)
{
    S9sOptions   *options = S9sOptions::instance();
    int           clusterId = options->clusterId();
    S9sVariantMap job;
    S9sRpcReply   reply;
    bool          success, finished;
    int           nLogsPrinted = 0;
    int           nEntries;

    //printf("\n");
    printf("\033[?25l"); 

    for (;;)
    {
        success = client.getJobLog(clusterId, jobId, nLogsPrinted);
        if (success)
        {
            reply = client.reply();
            success = reply.isOk();
        }
        
        if (!success)
            continue;

        nEntries = reply["messages"].toVariantList().size();
        
        if (nEntries > 0)
            reply.printJobLog();

        nLogsPrinted += nEntries;

        job = reply["job"].toVariantMap();
        finished = 
            job["status"] == "ABORTED"   ||
            job["status"] == "FINISHED"  ||
            job["status"] == "FAILED";
        
        fflush(stdout);
        if (finished)
            break;
        
        sleep(1);
    }

    printf("\033[?25h");
    printf("\n");
}
