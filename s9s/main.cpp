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
#include "library.h"
#include "S9sOptions"
#include "S9sRpcClient"
#include "S9sBusinessLogic"

#include <stdio.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

//
// FIXME: This is temporary, it will be much more complicated.
// 
int
perform_task()
{
    S9sOptions  *options = S9sOptions::instance();
    S9sString    controller = options->controller();
    int          port = options->controllerPort();
    S9sString    token = options->rpcToken();
    S9sRpcClient client(controller, port, token);

    S9S_WARNING("isClusterOperation() : %s",
            options->isClusterOperation() ? "true" : "false");

    S9S_WARNING("isRollingRestartRequested() : %s",
            options->isRollingRestartRequested() ? "true" : "false");

    if (options->isClusterOperation() && options->isListRequested())
    {
        S9S_WARNING("list");
        S9sRpcReply reply;
        bool        success;

        success = client.getClusters();
        if (success)
        {
            reply = client.reply();
            reply.printClusterList();
        } else {
            fprintf(stderr, "%s\n", STR(client.errorString()));
        }
    } else if (options->isClusterOperation() && options->isRollingRestartRequested())
    {
        int         clusterId = options->clusterId();
        S9sRpcReply reply;
        bool        success;

        success = client.rollingRestart(clusterId);
        if (success)
        {
            reply = client.reply();
            reply.printJobStarted();
        } else {
            fprintf(stderr, "ERROR: %s\n", STR(client.errorString()));
        }
    } else if (options->isNodeOperation() && options->isListRequested())
    {
        S9S_DEBUG("node list");

        S9sRpcReply reply;
        bool        success;

        success = client.getClusters();
        if (success)
        {
            reply = client.reply();
            reply.printNodeList();
        } else {
            fprintf(stderr, "%s\n", STR(client.errorString()));
        }
    } else if (options->isJobOperation() && options->isListRequested())
    {
        S9sRpcReply reply;
        int         clusterId = options->clusterId();
        bool        success;

        success = client.getJobInstances(clusterId);
        if (success)
        {
            reply = client.reply();
            reply.printJobList();
        } else {
            fprintf(stderr, "%s\n", STR(client.errorString()));
        }
    } else if (options->isJobOperation() && options->isLogRequested())
    {
        S9sRpcReply reply;
        int         clusterId = options->clusterId();
        int         jobId     = options->jobId();
        bool        success;

        success = client.getJobLog(clusterId, jobId);
        if (success)
        {
            reply = client.reply();
            reply.printJobLog();
        } else {
            fprintf(stderr, "%s\n", STR(client.errorString()));
        }
    }

    return 0;
}

int 
main(int argc, char **argv)
{
    S9sOptions *options = S9sOptions::instance();
    S9sBusinessLogic businessLogic;
    bool        success, finished;
    int         exitStatus;

    success = options->readOptions(&argc, argv);
    if (!success)
    {
        S9S_WARNING("Readoption failed.");
        if (!options->errorString().empty())
        {
            fprintf(stderr, "%s: %s\n\n", 
                    STR(options->binaryName()),
                    STR(options->errorString()));
            fflush(stderr);
        }

        goto finalize;
    }

    PRINT_VERBOSE("Command line options processed.");
    if (options->isVerbose())
    {
        printf("%sXTERM_COLOR_RED%s\n", XTERM_COLOR_RED, TERM_NORMAL);
        printf("%sXTERM_COLOR_GREEN%s\n", XTERM_COLOR_GREEN, TERM_NORMAL);
        printf("%sXTERM_COLOR_ORANGE%s\n", XTERM_COLOR_ORANGE, TERM_NORMAL);
        printf("%sXTERM_COLOR_BLUE%s\n", XTERM_COLOR_BLUE, TERM_NORMAL);
        printf("%sXTERM_COLOR_PURPLE%s\n", XTERM_COLOR_PURPLE, TERM_NORMAL);
        printf("%sXTERM_COLOR_CYAN%s\n", XTERM_COLOR_CYAN, TERM_NORMAL);
        printf("%sXTERM_COLOR_LIGHT_GRAY%s\n", XTERM_COLOR_LIGHT_GRAY, TERM_NORMAL);
        printf("%sXTERM_COLOR_DARK_GRAY%s\n", XTERM_COLOR_DARK_GRAY, TERM_NORMAL);
        printf("%sXTERM_COLOR_LIGHT_RED%s\n", XTERM_COLOR_LIGHT_RED, TERM_NORMAL);
        printf("%sXTERM_COLOR_LIGHT_GREEN%s\n", XTERM_COLOR_LIGHT_GREEN, TERM_NORMAL);
        printf("%sXTERM_COLOR_YELLOW%s\n", XTERM_COLOR_YELLOW, TERM_NORMAL);
        printf("%sXTERM_COLOR_LIGHT_BLUE%s\n", XTERM_COLOR_LIGHT_BLUE, TERM_NORMAL);
        printf("%sXTERM_COLOR_LIGHT_PURPLE%s\n", XTERM_COLOR_LIGHT_PURPLE, TERM_NORMAL);
        printf("%sXTERM_COLOR_LIGHT_CYAN%s\n", XTERM_COLOR_LIGHT_CYAN, TERM_NORMAL);
        printf("%sXTERM_COLOR_WHITE%s\n", XTERM_COLOR_WHITE, TERM_NORMAL);
    }

    finished = options->executeInfoRequest();
    if (finished)
        goto finalize;

    //perform_task();
    businessLogic.execute();

finalize:
    exitStatus = options->exitStatus();
    PRINT_VERBOSE("Exiting with exitcode %d.", exitStatus);

    S9sOptions::uninit();

    return exitStatus;
}

