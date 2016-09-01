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
#include "S9sDateTime"

#define DEBUG
#define WARNING
#include "s9sdebug.h"

void 
S9sRpcReply::printJobList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (options->isLongRequested())
        printJobListLong();
    else
        printJobListBrief();
}

void 
S9sRpcReply::printNodeList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (options->isLongRequested())
        printNodeListLong();
    else
        printNodeListBrief();
}


void 
S9sRpcReply::printClusterList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (options->isLongRequested())
        printClusterListLong();
    else
        printClusterListBrief();
}

        
void 
S9sRpcReply::printClusterListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("clusters").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap = theList[idx].toVariantMap();
        S9sString     clusterName = theMap["cluster_name"].toString();

        if (syntaxHighlight)
            printf("%s%s%s ", TERM_BLUE, STR(clusterName), TERM_NORMAL);
        else
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
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();

    //printf("%s", STR(toString()));

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
        
        if (syntaxHighlight)
        {
            printf("%c%c%c %4d %-14s %s%-20s%s\n", 
                    stateFlagFromState(state),
                    cRecovery ? 'c' : '-',
                    nRecovery ? 'n' : '-',
                    clusterId, 
                    STR(clusterType.toLower()),
                    TERM_BLUE, STR(clusterName), TERM_NORMAL);
        } else {
            printf("%c%c%c %4d %-14s %-20s\n", 
                    stateFlagFromState(state),
                    cRecovery ? 'c' : '-',
                    nRecovery ? 'n' : '-',
                    clusterId, 
                    STR(clusterType.toLower()),
                    STR(clusterName));
        }
    }
}

void 
S9sRpcReply::printNodeListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("clusters").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sString      clusterName = theMap["cluster_name"].toString();
        S9sVariantList hostList = theMap["hosts"].toVariantList();

        for (uint idx2 = 0; idx2 < hostList.size(); ++idx2)
        {
            S9sVariantMap hostMap = hostList[idx2].toVariantMap();
            S9sString     hostName = hostMap["hostname"].toString();
            S9sString     status = hostMap["hoststatus"].toString();

            if (syntaxHighlight)
            {
                if (status == "CmonHostOnline")
                    printf("%s%s%s ", TERM_GREEN, STR(hostName), TERM_NORMAL);
                else if (status == "CmonHostRecovery")
                    printf("%s%s%s ", TERM_YELLOW, STR(hostName), TERM_NORMAL);
                else 
                    printf("%s%s%s ", TERM_RED, STR(hostName), TERM_NORMAL);
            } else {
                printf("%s ", STR(hostName));
            }
        }
    }

    printf("\n");
}

void 
S9sRpcReply::printNodeListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();

    printf("%s", STR(toString()));

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
        
        if (syntaxHighlight)
        {
            printf("%c%c%c %4d %-14s %s%-20s%s %s\n", 
                    stateFlagFromState(state),
                    cRecovery ? 'c' : '-',
                    nRecovery ? 'n' : '-',
                    clusterId, 
                    STR(clusterType.toLower()),
                    TERM_BLUE, STR(clusterName), TERM_NORMAL,
                    STR(text));
        } else {
            printf("%c%c%c %4d %-14s %-20s %s\n", 
                    stateFlagFromState(state),
                    cRecovery ? 'c' : '-',
                    nRecovery ? 'n' : '-',
                    clusterId, 
                    STR(clusterType.toLower()),
                    STR(clusterName),
                    STR(text));
        }
    }
}

void 
S9sRpcReply::printJobListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("jobs").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    int             total = operator[]("total").toInt();

    printf("Total: %d\n", total);

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        int            jobId  = theMap["job_id"].toInt();
        S9sString      status = theMap["status"].toString();
        S9sString      title  = theMap["title"].toString();
        S9sString      user   = theMap["user_name"].toString();
        S9sString      percent;
        S9sDateTime    created;
        S9sString      timeStamp;
        const char    *stateColorStart = "";
        const char    *stateColorEnd   = "";

        if (user.empty())
            user.sprintf("%d", theMap["user_id"].toInt());

        // The progress.
        if (theMap.contains("progress_percent"))
        {
            double value = theMap["progress_percent"].toDouble();

            percent.sprintf("%3.0f%%", value);
        } else if (status == "FINISHED") 
        {
            percent = "100%";
        } else {
            percent = "???%";
        }

        // The timestamp.
        created.parse(theMap["created"].toString());
        timeStamp = created.toString(S9sDateTime::MySqlLogFileFormat);

        if (syntaxHighlight)
        {
            if (status == "RUNNING" || status == "RUNNING_EXT")
            {
                stateColorStart = XTERM_COLOR_9;
                stateColorEnd   = TERM_NORMAL;
            } else if (status == "FINISHED")
            {
                stateColorStart = XTERM_COLOR_9;
                stateColorEnd   = TERM_NORMAL;
            } else if (status == "FAILED")
            {
                stateColorStart = XTERM_COLOR_1;
                stateColorEnd   = TERM_NORMAL;
            }
        }

        printf("%5d ", jobId);
        printf("%s%-10s%s ", stateColorStart, STR(status), stateColorEnd);
        printf("%-8s ", STR(user));
        printf("%s ", STR(timeStamp));
        printf("%s ", STR(percent));
        printf("%s\n", STR(title));
    }
}


void 
S9sRpcReply::printJobListLong()
{
    printf("TBD\n");
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
