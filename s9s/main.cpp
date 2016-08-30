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

#include <stdio.h>

//#define DEBUG
#define WARNING
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

    if (options->isListRequested())
    {
        S9sRpcReply reply;

        client.getClusters();
        reply = client.reply();
        reply.printClusterList();
    }

    return 0;
}


int main(int argc, char **argv)
{
    S9sOptions *options = S9sOptions::instance();
    bool        success, finished;
    int         exitStatus;

    success = options->readOptions(&argc, argv);
    if (!success)
    {
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

    finished = options->executeInfoRequest();
    if (finished)
        goto finalize;

    perform_task();

finalize:
    exitStatus = options->exitStatus();
    PRINT_VERBOSE("Exiting with exitcode %d.", exitStatus);

    S9sOptions::uninit();

    return exitStatus;
}

