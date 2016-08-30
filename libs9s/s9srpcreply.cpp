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
#include "s9srpcreply.h"

#include <stdio.h>

#include "S9sOptions"

#define DEBUG
#define WARNING
#include "s9sdebug.h"



void 
S9sRpcReply::printClusterList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isLongRequested())
        printClusterListLong();
    else
        printClusterListBrief();
}

        
void 
S9sRpcReply::printClusterListBrief()
{
    S9sVariantList theList = operator[]("clusters").toVariantList();

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap = theList[idx].toVariantMap();
        S9sString     clusterName = theMap["cluster_name"].toString();

        printf("%s ", STR(clusterName));
    }

    printf("\n");
}

/*
s9s cluster --list --long  --controller=localhost --controller-port=9555 --rpc-token="kljkj" 
{
    "cc_timestamp": 1472546799,
    "clusters": [ 
    {
        "alarm_statistics": 
        {
            "class_name": "CmonAlarmStatistics",
            "cluster_id": 200,
            "critical": 3,
            "warning": 1
        },
        "class_name": "CmonClusterInfo",
        "cluster_auto_recovery": true,
        "cluster_id": 200,
        "cluster_name": "default_repl_200",
        "cluster_type": "MYSQLCLUSTER",
        "configuration_file": "configs/UtS9sCluster_01.conf",
        "job_statistics": 
        {
            "by_state": 
            {
                "ABORTED": 0,
                "DEFINED": 0,
                "DEQUEUED": 0,
                "FAILED": 0,
                "FINISHED": 0,
                "RUNNING": 0
            },
            "class_name": "CmonJobStatistics",
            "cluster_id": 200
        },
        "log_file": "./cmon-ut-communication.log",
        "maintenance_mode_active": false,
        "managed": true,
        "node_auto_recovery": true,
        "state": "MGMD_NO_CONTACT",
        "status_text": "No contact to the management node.",
        "vendor": "oracle",
        "version": "5.5"
    } ],
    "requestStatus": "ok"

 */
void 
S9sRpcReply::printClusterListLong()
{
    //printf("%s", STR(toString()));
#if 1
    S9sVariantList theList = operator[]("clusters").toVariantList();

    printf("Total: %lu\n", theList.size());
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap      = theList[idx].toVariantMap();
        S9sString     clusterName = theMap["cluster_name"].toString();
        int           clusterId   = theMap["cluster_id"].toInt();
        S9sString     clusterType = theMap["cluster_type"].toString();
        S9sString     state       = theMap["state"].toString();
        bool          cRecovery   = theMap["cluster_auto_recovery"].toBoolean();
        bool          nRecovery   = theMap["node_auto_recovery"].toBoolean();
        S9sString     text        = theMap["status_text"].toString();
        
        printf("%c%c%c %4d %-14s %-20s %s\n", 
                stateFlagFromState(state),
                cRecovery ? 'c' : '-',
                nRecovery ? 'n' : '-',
                clusterId, 
                STR(clusterType.toLower()),
                STR(clusterName),
                STR(text));
    }
#endif
}

char 
S9sRpcReply::stateFlagFromState(
        const S9sString &state)
{
    if (state == "MGMD_NO_CONTACT")
        return 'n';
    else if (state == "STARTED")
        return 's';
    else if (state == "NOT_STARTED")
        return 'F';
    else if (state == "DEGRADED")
        return 'd';
    else if (state == "FAILURE")
        return 'f';
    else if (state == "SHUTTING_DOWN")
        return 'w';
    else if (state == "RECOVERING")
        return 'r';
    else if (state == "STARTING")
        return 'S';
    else if (state == "UNKNOWN")
        return '-';
    else if (state == "STOPPED")
        return 't';

    return '?';
}
