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
 * S9sTools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S9sTools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9stopui.h"

#include "S9sRpcClient"
#include "S9sOptions"
#include "S9sDateTime"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define DEBUG
#define WARNING
#include "s9sdebug.h"
        
struct termios orig_termios;

S9sTopUi::S9sTopUi()
{
}

S9sTopUi::~S9sTopUi()
{
}

void
S9sTopUi::executeTop(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    int          clusterId = options->clusterId();
    bool         success;

    if (clusterId <= 0)
    {
        PRINT_ERROR("The cluster ID is invalid while executing 'top'.");
        return;
    }

    printf("%s", TERM_CLEAR_SCREEN);

    for (;;)
    {
        success = executeTopOnce(client);

        if (!success)
            break;
        
        sleep(options->updateFreq());
    }
}

bool
S9sTopUi::executeTopOnce(
        S9sRpcClient &client)
{
    S9sOptions  *options = S9sOptions::instance();
    int          clusterId = options->clusterId();
    S9sString    clusterName, clusterStatusText;
    S9sRpcReply  reply;
    bool         success = true;
    S9sDateTime  date = S9sDateTime::currentDateTime();
    S9sString    dateString = date.toString(S9sDateTime::LongTimeFormat);
    int          terminalWidth = options->terminalWidth();
    int          columns;
    S9sString    tmp;

    //
    // The cluster information.
    //
    success = client.getCluster(clusterId);
    reply = client.reply();
    if (!success)
        return success;

    clusterName = reply.clusterName(clusterId);
    clusterStatusText = reply.clusterStatusText(clusterId);
        
    columns  = terminalWidth;
    columns -= clusterName.length();
    columns -= clusterStatusText.length();
    columns -= 12;
        
    tmp = S9sString::space * columns;

    printf("\033[0;0H");
    //printf("columns: %d\n", columns);
    printf("%s - %s ", STR(clusterName), STR(dateString));
    printf("%s", STR(tmp));
    printf("%s", STR(clusterStatusText));
    printf("\n");

    //
    // Summary of CPU usage.
    //
    client.getCpuStats(clusterId);
    reply = client.reply();

    reply.printCpuStatLine1();
   
    //
    // The memory summary.
    //
    client.getMemoryStats(clusterId);
    reply = client.reply();
    reply.printMemoryStatLine1();
    reply.printMemoryStatLine2();
    printf("\n");

    //
    // List of processes.
    //
    client.getRunningProcesses(clusterId);
    reply = client.reply();

    //S9S_DEBUG("reply.printProcessList(%d)", options->terminalHeight() - 6);
    reply.printProcessList(options->terminalHeight() - 6);


    if (!success)
    {
        PRINT_ERROR("%s", STR(client.errorString()));
    }

    return success;
}

