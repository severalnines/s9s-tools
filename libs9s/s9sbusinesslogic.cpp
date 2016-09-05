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
        executeCreateCluster(client);
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

            if (options->isWaitRequested())
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
S9sBusinessLogic::executeCreateCluster(
        S9sRpcClient &client)
{
    S9sOptions    *options = S9sOptions::instance();
    //int            clusterId = options->clusterId();
    S9sVariantList hostNames;
    S9sString      osUserName;
    S9sString      vendor;
    S9sString      mySqlVersion;
    bool           uninstall = true;
    S9sRpcReply    reply;
    bool           success;

    success = client.createGaleraCluster(
            hostNames, osUserName, vendor, mySqlVersion, uninstall);
    if (success)
    {
        reply = client.reply();

        success = reply.isOk();
        if (success)
        {

            if (options->isWaitRequested())
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
S9sBusinessLogic::waitForJob(
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
