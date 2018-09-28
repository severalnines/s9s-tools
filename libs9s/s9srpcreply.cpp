/*
 * Severalnines Tools
 * Copyright (C) 2017-2018 Severalnines AB
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
#include "s9srpcreply.h"

#include <stdio.h>

#include "S9sOptions"
#include "S9sDateTime"
#include "S9sFile"
#include "S9sFormat"
#include "S9sRegExp"
#include "S9sNode"
#include "S9sCluster"
#include "S9sAlarm"
#include "S9sBackup"
#include "S9sMessage"
#include "S9sCmonGraph"
#include "S9sTreeNode"
#include "S9sUser"
#include "S9sAccount"
#include "S9sGroup"
#include "S9sReport"
#include "S9sServer"
#include "S9sJob"
#include "S9sContainer"
#include "S9sStringList"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

#define BoolToHuman(boolVal) ((boolVal) ? 'y' : 'n')

S9sRpcReply::S9sRpcReply() :
    m_numberOfObjects(0),
    m_numberOfFolders(0)
{
}

S9sRpcReply::ErrorCode
S9sRpcReply::requestStatus() const
{
    S9sString errorCodeString = "ok";
    ErrorCode retval = Ok;

    if (contains("requestStatus"))
        errorCodeString = at("requestStatus").toString().toLower();
    else if (contains("request_status"))
        errorCodeString = at("request_status").toString().toLower();

    if (errorCodeString == "ok")
        retval = Ok;
    else if (errorCodeString == "invalidrequest")
        retval = InvalidRequest;
    else if (errorCodeString == "tryagain")
        retval = TryAgain;
    else if (errorCodeString == "clusternotfound")
        retval = ClusterNotFound;
    else if (errorCodeString == "unknownerror")
        retval = UnknownError;
    else if (errorCodeString == "accessdenied")
        retval = AccessDenied;
    else if (errorCodeString == "authrequeired")
        retval = AuthRequired;
    else if (errorCodeString == "connecterror")
        retval = ConnectError;
    else 
        retval = UnknownError;

    return retval;
}

S9sString
S9sRpcReply::requestStatusAsString() const
{
    S9sString errorCodeString = "???";

    if (contains("requestStatus"))
        errorCodeString = at("requestStatus").toString().toLower();
    else if (contains("request_status"))
        errorCodeString = at("request_status").toString().toLower();

    return errorCodeString;
}


S9sRpcReply &
S9sRpcReply::operator=(
        const S9sVariantMap &theMap)
{
    S9sVariantMap::operator=(theMap);
    return *this;
}

/**
 * \returns true if the reply states that the request status is 'ok'.
 */
bool
S9sRpcReply::isOk() const
{
    // One is for RPC 1.0, the other is for RPC 2.0
    if (contains("requestStatus"))
        return at("requestStatus").toString().toLower() == "ok";
    else if (contains("request_status"))
        return at("request_status").toString().toLower() == "ok";

    return false;
}

/**
 * \returns true if the reply states that authentication is required.
 */
bool
S9sRpcReply::isAuthRequired() const
{
    // One is for RPC 1.0, the other is for RPC 2.0
    if (contains("requestStatus"))
        return at("requestStatus").toString().toLower() == "authrequired";
    else if (contains("request_status"))
        return at("request_status").toString().toLower() == "authrequired";

    return false;
}

/**
 * \returns The error string sent by the controller in the reply if there is
 *   indeed an error string.
 */
S9sString
S9sRpcReply::errorString() const
{
    // One for RPC 1.0, the other is for RPC 2.0
    if (contains("errorString"))
        return at("errorString").toString();
    else if (contains("error_string"))
        return at("error_string").toString();

    return S9sString();
}

S9sString
S9sRpcReply::uuid() const
{
    // One for RPC 1.0, the other is for RPC 2.0
    if (contains("UUID"))
        return at("UUID").toString();

    return S9sString();
}

/**
 * \returns The variant list that contains variant maps with the jobs in the
 *   reply.
 *
 * It is either one job coming from the "job" field of the reply or zero to many
 * maps coming from the "jobs" field. This depends on the request we sent.
 */
S9sVariantList 
S9sRpcReply::jobs()
{
    S9sVariantList retval;

    if (contains("job"))
        retval << operator[]("job").toVariantMap();
    else if (contains("jobs"))
        retval = operator[]("jobs").toVariantList();

    return retval;
}

/**
 * \returns the job ID from the reply if the reply contains a job ID, returns -1
 *   otherwise.
 *
{
    "cc_timestamp": 1472800942,
    "job": 
    {
        "can_be_aborted": false,
        "can_be_deleted": true,
        "class_name": "CmonJobInstance",
        "cluster_id": 1,
        "created": "2016-09-02T07:21:53.000Z",
        "exit_code": 0,
        "job_id": 16,
        "job_spec": "{\n    \"command\": \"rolling_restart\"\n}",
        "status": "DEFINED",
        "status_text": "Waiting",
        "title": "Rolling Restart",
        "user_id": 1000,
        "user_name": "pipas"
    },
    "requestStatus": "ok"
}
*/
int
S9sRpcReply::jobId() const
{
    S9sVariantMap job;
    int           retval = -1;

    if (!contains("job"))
        return retval;

    job = at("job").toVariantMap();
    retval = job["job_id"].toInt();

    return retval;
}

S9sString
S9sRpcReply::jobTitle() const
{
    S9sVariantMap job;
    S9sString     retval;

    if (!contains("job"))
        return retval;

    job = at("job").toVariantMap();
    retval = job["title"].toString();

    return retval;
}

/**
 * \returns true if the reply contains a job and that job's status is "FAILED"
 */
bool
S9sRpcReply::isJobFailed() const
{
    S9sVariantMap job;
    bool          retval = false;

    if (!contains("job"))
        return retval;

    job = at("job").toVariantMap();
    retval = job["status"].toString() == "FAILED";

    return retval;
}

/**
 * \param clusterName The name of the cluster to return.
 * \returns The cluster object from the reply.
 */
S9sCluster
S9sRpcReply::cluster(
        const S9sString &clusterName)
{
    S9sVariantList clusterList = clusters();

    for (uint idx = 0u; idx < clusterList.size(); ++idx)
    {
        S9sCluster thisCluster(clusterList[idx].toVariantMap());

        if (thisCluster.name() == clusterName)
            return thisCluster;
    }

    return S9sCluster();
}

/**
 * The reply to getClusterInfo and getAllClusterInfo replies are similar, one
 * contains a single cluster, the other contains a list of clusters. This
 * function returns the cluster(s) as a list of maps no matter which way the
 * reply is built.
 */
S9sVariantList
S9sRpcReply::clusters()
{
    S9sVariantList  theList;
    
    if (contains("clusters"))
        theList = operator[]("clusters").toVariantList();
    else if (contains("cluster"))
        theList << operator[]("cluster");

    return theList;
}

S9sVariantList
S9sRpcReply::alarms()
{
    S9sVariantList  theList;
    
    if (contains("alarms"))
        theList = operator[]("alarms").toVariantList();

    return theList;
}

void
S9sRpcReply::printCat()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
    } else if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
    } else {
        S9sString content = operator[]("file_content").toString();
        printf("%s", STR(content));
    }
}

void
S9sRpcReply::printAcl()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
    } else if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
    } else {
        S9sString acl = operator[]("acl").toString();
        S9sString owner = operator[]("owner_user_name").toString();
        S9sString group = operator[]("owner_group_name").toString();
        S9sString name  = operator[]("object_name").toString();

        if (!name.empty())
            printf("# name: %s\n", STR(name));

        if (!owner.empty())
            printf("# owner: %s\n", STR(owner));
        
        if (!group.empty())
            printf("# group: %s\n", STR(group));

        acl.replace(",", "\n");
        printf("%s\n", STR(acl));
    }
    
}

/**
 * Prints the RPC reply as a cluster list. Considers command line options to
 * decide what format will be used to print the list.
 */
void 
S9sRpcReply::printClusterList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (!isOk())
        PRINT_ERROR("%s", STR(errorString()));
    else if (options->isStatRequested())
        printClusterListStat();
    else if (options->isLongRequested())
        printClusterListLong();
    else
        printClusterListBrief();
}

S9sString
S9sRpcReply::clusterName(
        const int clusterId)
{
    S9sVariantMap theMap = clusterMap(clusterId);

    return theMap["cluster_name"].toString();
}

S9sString
S9sRpcReply::clusterStatusText(
        const int clusterId)
{
    S9sVariantMap theMap = clusterMap(clusterId);

    return theMap["status_text"].toString();
}

/**
 * \returns true if the job is finished (or aborted or failed) and so monitoring
 *   it should be aborted too.
 *
{
    "cc_timestamp": 1472802884,
    "job": 
    {
        "can_be_aborted": false,
        "can_be_deleted": true,
        "class_name": "CmonJobInstance",
        "cluster_id": 1,
        "created": "2016-09-02T07:53:53.000Z",
        "exit_code": 0,
        "job_id": 21,
        "job_spec": "{\n    \"command\": \"rolling_restart\"\n}",
        "status": "DEFINED",
        "status_text": "Waiting",
        "title": "Rolling Restart",
        "user_id": 1000,
        "user_name": "pipas"
    },
    "requestStatus": "ok"
}

    "cc_timestamp": 1476944158,
    "job": 
    {
        "can_be_aborted": false,
        "can_be_deleted": false,
        "class_name": "CmonJobInstance",
        "cluster_id": 0,
        "created": "2016-10-20T06:15:36.000Z",
        "exit_code": 0,
        "has_progress": true,
        "job_id": 1,
        "job_spec": "{\n    \"command\": \"create_cluster\",\n    \"job_data\": \n    {\n        \"cluster_name\": \"ft_galera_58604\",\n        \"cluster_type\": \"galera\",\n        \"enable_mysql_uninstall\": true,\n        \"mysql_hostnames\": [ \"192.168.1.104\", \"192.168.1.181\", \"192.168.1.182\" ],\n        \"mysql_password\": \"\",\n        \"mysql_version\": \"5.6\",\n        \"ssh_user\": \"pipas\",\n        \"vendor\": \"percona\"\n    }\n}",
        "progress_percent": 6,
        "started": "2016-10-20T06:15:39.000Z",
        "status": "RUNNING",
        "status_text": "Setting up 192.168.1.104",
        "title": "Create Galera Cluster",
        "user_id": 0,
        "user_name": "pipas"
    },
    "requestStatus": "ok"
}
*/
bool 
S9sRpcReply::progressLine(
        S9sString &retval,
        bool       syntaxHighlight)
{
    S9sVariantMap job = operator[]("job").toVariantMap();
    int           jobId;
    S9sString     status;
    double        percent;
    bool          hasProgress;
    S9sString     tmp;
    S9sString     statusText;

    retval.clear();

    if (job.empty())
        return false;

    // The job id.
    jobId = job["job_id"].toInt();
    tmp.sprintf("Job %2d ", jobId);
    retval += tmp;

    // The status
    status = job["status"].toString();
    tmp.sprintf("%-10s ", STR(status));
    if (syntaxHighlight)
    {
        if (status.startsWith("RUNNING") || status == "FINISHED")
            retval += XTERM_COLOR_GREEN;
        else if (status == "FAILED" || status == "ABORTED")
            retval += XTERM_COLOR_RED;
    }

    retval += tmp;
    
    if (syntaxHighlight)
        retval += TERM_NORMAL;

    // The progress bar.
    hasProgress = job.contains("progress_percent");
    if (status == "FINISHED")
    {
        percent = 100.0;
        hasProgress = true;
    }

    if (hasProgress)
    {
        percent = job["progress_percent"].toDouble();
        if (status == "FINISHED")
            percent = 100.0;

        retval += progressBar(percent, syntaxHighlight);
    } else if (status.startsWith("RUNNING"))
    {
        retval += progressBar(syntaxHighlight);
    } else {
        // XTERM_COLOR_LIGHT_GRAY
        // TERM_NORMAL
        retval += "[----------] ";
    }

    // percent string.
    if (hasProgress)
    {
        tmp.sprintf("%3.0f%% ", percent);
        retval += tmp;
    } else {
        retval += "---% ";
    }

    // Status text.
    if (syntaxHighlight)
        retval += TERM_BOLD;

    statusText = job["status_text"].toString();
    statusText = S9sString::html2ansi(statusText);
    
    retval += statusText;
    retval += "      ";
    
    if (syntaxHighlight)
        retval += TERM_NORMAL;

    return 
        status == "ABORTED"  ||
        status == "FINISHED" ||
        status == "FAILED";
}

void
S9sRpcReply::printMessages(
        const S9sString &defaultMessage)
{
    S9sOptions    *options = S9sOptions::instance();
        
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    if (options->isBatchRequested())
        return;

    // Print out the messages first (or the default message)
    if (contains("messages"))
    {
        // RPC 2.0 might hold multiple messages.
        S9sVariantList list = at("messages").toVariantList();

        for (uint idx = 0u; idx < list.size(); ++idx)
        {
            printf("%s\n", STR(list[idx].toString()));
        }
    }
    else if (errorString().empty())
    {
        // no messages, and no error string, print out the default
        if (isOk())
        {
            printf("%s\n", STR(defaultMessage));
        }
        else
        {
            PRINT_ERROR("Error: Unknown error: %s\n", STR(toString()));
        }
    }

    // And if error string is set, pint out it as well
    if (!errorString().empty())
    {
        if (isOk())
        {
            printf("%s\n", STR(errorString()));
        }
        else
        {
            PRINT_ERROR("%s\n", STR(errorString()));
        }
    }
}

void
S9sRpcReply::printCheckHostsReply()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  hosts = operator[]("checked_hosts").toVariantList();;
   
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    for (uint idx = 0u; idx < hosts.size(); ++idx)
    {
        S9sVariantMap  theMap = hosts[idx].toVariantMap();
        S9sNode        node   = theMap["host"].toVariantMap();
        S9sVariantMap  status = theMap["status"].toVariantMap();
        S9sString      message = status["error_message"].toString();
        S9sString      errorCode = status["error_code"].toString();
        const char    *hostColorBegin = "";
        const char    *hostColorEnd   = "";

        if (syntaxHighlight)
        {
            hostColorBegin  = XTERM_COLOR_GREEN;
            hostColorEnd    = TERM_NORMAL;
        }

        if (message.empty())
            message = "-";

        printf("%s ", errorCode == "HostIsOk" ?  "SUCCESS" : "FAILURE");
        printf("%s%s%s ", hostColorBegin, STR(node.hostName()), hostColorEnd);
        printf("%s", STR(message));
        printf("\n");
    }
}

/**
 * This is a simple output function that we can call to print a short message
 * when a new job is registered on the server. This prints the job ID that is
 * essential for the user to later monitor the job. 
 */
void 
S9sRpcReply::printJobStarted()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            isBatch = options->isBatchRequested();
    int             id;

    if (options->isJsonRequested())
    {
        printf("%s", STR(toString()));
        return;
    }

    if (isOk())
    {
        S9sVariantMap job = operator[]("job").toVariantMap();;

        if (job.empty())
        {
            // This should not happen, it is a deprecated reply format.
            id = operator[]("jobId").toInt();
        } else {
            id = job["job_id"].toInt();
        }
            
        if (isBatch)
            printf("%d\n", id);
        else
            printf("Job with ID %d registered.\n", id);
    } else {
        printf("%s", STR(toString()));
    }
}

/**
 * Prints log messages if they are in the RPC reply.
 */
void
S9sRpcReply::printJobLog()
{
    S9sOptions     *options = S9sOptions::instance();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    } else if (options->isLongRequested())
    {
        printJobLogLong();
    } else {
        printJobLogBrief();
    }
}

void
S9sRpcReply::printPing()
{
    S9sString requestStatus;
    S9sString requestCreated, replyReceived;

    if (contains("requestStatus"))
        requestStatus = at("requestStatus").toString().toUpper();
    else if (contains("request_status"))
        requestStatus = at("request_status").toString().toUpper();
    else 
        requestStatus = "UNKNOWN";

    if (contains("request_created"))
        requestCreated = at("request_created").toString();

    if (contains("reply_received"))
        replyReceived  = at("reply_received").toString();

    printf("PING ");
    printf("%s ", STR(requestStatus));

    if (!requestCreated.empty() && !replyReceived.empty())
    {
        S9sDateTime start, end;

        if (start.parse(requestCreated) && end.parse(replyReceived))
        {
            double millisec = S9sDateTime::milliseconds(end, start);

            printf("%.3f ms", millisec);
            #if 0
            printf("\n");
            printf("%s\n", STR(requestCreated));
            printf("%s\n", STR(replyReceived));
            #endif
        }
    }

    printf("\n");
}

bool 
compareProcessByCpuUsage(
        const S9sVariant &a,
        const S9sVariant &b)
{
    S9sVariantMap aMap = a.toVariantMap();
    S9sVariantMap bMap = b.toVariantMap();

    return aMap["cpu_usage"].toDouble() > bMap["cpu_usage"].toDouble();
}

bool 
compareProcessByMemoryUsage(
        const S9sVariant &a,
        const S9sVariant &b)
{
    S9sVariantMap aMap = a.toVariantMap();
    S9sVariantMap bMap = b.toVariantMap();

    return aMap["res_mem"].toULongLong() > bMap["res_mem"].toULongLong();
}

void
S9sRpcReply::printProcessList()
{
    S9sOptions  *options = S9sOptions::instance();

    if (options->isJsonRequested())
    {
        printf("\n%s\n", STR(toString()));
        return;
    } else if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
        return;
    }

    // FIXME: no brief version...
    printProcessListLong();
}

/**
 * Prints the process list in its long and detailed format.
 */
void
S9sRpcReply::printProcessListLong(
        const int maxLines)
{
    S9sOptions     *options = S9sOptions::instance();
    int             nItemsLimit = options->limit();
    int             terminalWidth = options->terminalWidth();
    int             columns;
    S9sVariantList  hostList = operator[]("data").toVariantList();
    S9sVariantList  processList;
    S9sFormat       hostFormat;
    S9sFormat       userFormat;
    S9sFormat       pidFormat;
    S9sFormat       priorityFormat;
    S9sFormat       virtFormat;
    S9sFormat       resFormat;
    int             nItems   = 0;
    int             nTotal   = 0;
    int             nRunning = 0;
    int             nHosts   = 0;

    /*
     * Go through the data and collect information.
     */
    for (uint idx = 0u; idx < hostList.size(); ++idx)
    {
        S9sString hostName = hostList[idx]["hostname"].toString();
        S9sVariantList processes = hostList[idx]["processes"].toVariantList();
    
        for (uint idx1 = 0u; idx1 < processes.size(); ++idx1)
        {
            S9sVariantMap process = processes[idx1].toVariantMap();
            
            process["hostname"] = hostName;
            
            ++nTotal;
            if (process["state"].toString() == "R")
                nRunning++;

            processList << process;
        }

        nHosts++;
    }
    
    /**
     * Sorting...
     */
    if (options->getBool("sort_by_memory"))
    {
        sort(processList.begin(), processList.end(), 
                compareProcessByMemoryUsage);

    } else {
        sort(processList.begin(), processList.end(), 
                compareProcessByCpuUsage);
    }

    /*
     * Again, now collecting format information.
     */
    for (uint idx = 0u; idx < processList.size(); ++idx)
    {
        S9sVariantMap process    = processList[idx].toVariantMap();
        int           pid        = process["pid"].toInt();
        S9sString     user       = process["user"].toString();
        S9sString     hostName   = process["hostname"].toString();
        int           priority   = process["priority"].toInt();
        ulonglong     rss        = process["res_mem"].toULongLong();
        ulonglong     virtMem    = process["virt_mem"].toULongLong();
        S9sString     executable = process["executable"].toString();

        if (maxLines > 0 && (int) idx >= maxLines)
            break;
        
        if (!options->isStringMatchExtraArguments(executable))
            continue;

        pidFormat.widen(pid);
        userFormat.widen(user);
        hostFormat.widen(hostName);
        priorityFormat.widen(priority);
        virtFormat.widen(virtMem);
        resFormat.widen(rss);

        ++nItems;;
        if (nItemsLimit > 0 && nItems >= nItemsLimit)
            break;
    }

    /*
     * The header.
     */
    columns  = terminalWidth;
    columns -= pidFormat.realWidth();
    columns -= userFormat.realWidth();
    columns -= hostFormat.realWidth();
    columns -= priorityFormat.realWidth();
    columns -= 45;

    virtFormat.setRightJustify();
    resFormat.setRightJustify();

    if (!options->isNoHeaderRequested())
    {    
        pidFormat.widen("PID");
        userFormat.widen("USER");
        hostFormat.widen("HOST");
        priorityFormat.widen("PR");
        virtFormat.widen("VIRT");
        resFormat.widen("RES");

        printf("%s", headerColorBegin());

        pidFormat.printf("PID");
        userFormat.printf("USER");
        hostFormat.printf("HOST");
        priorityFormat.printf("PR");
        virtFormat.printf("VIRT");
        resFormat.printf("RES");

        ::printf("%s", "S   %CPU   %MEM COMMAND    ");
        ::printf("%s", TERM_NORMAL);
        ::printf("\n");
    }

    // rss          resident set size
    // vsz          virtual memory size
    nItems = 0;
    for (uint idx = 0u; idx < processList.size(); ++idx)
    {
        S9sVariantMap process    = processList[idx].toVariantMap();
        int           pid        = process["pid"].toInt();
        S9sString     hostName   = process["hostname"].toString();
        S9sString     user       = process["user"].toString();
        S9sString     executable = process["executable"].toString();
        double        cpuUsage   = process["cpu_usage"].toDouble();
        double        memUsage   = process["mem_usage"].toDouble();
        S9sString     state      = process["state"].toString();
        ulonglong     rss        = process["res_mem"].toULongLong();
        ulonglong     virtMem    = process["virt_mem"].toULongLong();
        int           priority   = process["priority"].toInt();
        
        rss     /= 1024;
        virtMem /= 1024;

        if (!options->isStringMatchExtraArguments(executable))
            continue;

        pidFormat.printf(pid);

        printf("%s", userColorBegin());        
        userFormat.printf(user);
        printf("%s", userColorEnd());
        
        printf("%s", serverColorBegin());
        hostFormat.printf(hostName);
        printf("%s", serverColorEnd()); 

        priorityFormat.printf(priority);

        virtFormat.printf(m_formatter.kiloBytesToHuman(virtMem));
        resFormat.printf(m_formatter.kiloBytesToHuman(rss));

        if (state.length() == 1u)
            ::printf("%1s ", STR(state));
        else 
            ::printf("? ");

        printf("%6.2f ", cpuUsage);
        printf("%6.2f ", memUsage); 
        
        ::printf("%s", executableColorBegin(executable));
        ::printf("%s", STR(executable));
        ::printf("%s", executableColorEnd()); 

        printf("\n");

        ++nItems;;
        if (nItemsLimit > 0 && nItems >= nItemsLimit)
            break;
    }
        
    if (!options->isBatchRequested())
    {
        ::printf("Total: %s%'d%s processes on %s%d%s host(s),"
                " %s%d%s running.\n", 
                numberColorBegin(), nTotal, numberColorEnd(),
                numberColorBegin(), nHosts, numberColorEnd(),
                numberColorBegin(), nRunning, numberColorEnd());
    }
}

void
S9sRpcReply::printProcessListTop(
        const int maxLines)
{
    S9sOptions     *options = S9sOptions::instance();
    int             terminalWidth = options->terminalWidth();
    int             columns;
    S9sVariantList  hostList = operator[]("data").toVariantList();
    S9sVariantList  processList;
    S9sFormat       hostFormat;
    S9sFormat       userFormat;
    S9sFormat       pidFormat;
    S9sFormat       priorityFormat;

    /*
     * Go through the data and collect information.
     */
    for (uint idx = 0u; idx < hostList.size(); ++idx)
    {
        S9sString hostName = hostList[idx]["hostname"].toString();
        S9sVariantList processes = hostList[idx]["processes"].toVariantList();
    
        for (uint idx1 = 0u; idx1 < processes.size(); ++idx1)
        {
            S9sVariantMap process = processes[idx1].toVariantMap();

            process["hostname"] = hostName;
            processList << process;
        }
    }
    
    sort(processList.begin(), processList.end(), compareProcessByCpuUsage);

    /*
     * Again, now collecting format information.
     */
    for (uint idx = 0u; idx < processList.size(); ++idx)
    {
        S9sVariantMap process    = processList[idx].toVariantMap();
        int           pid        = process["pid"].toInt();
        S9sString     user       = process["user"].toString();
        S9sString     hostName   = process["hostname"].toString();
        int           priority   = process["priority"].toInt();

        if (maxLines > 0 && (int) idx >= maxLines)
            break;

        pidFormat.widen(pid);
        userFormat.widen(user);
        hostFormat.widen(hostName);
        priorityFormat.widen(priority);
    }

    /*
     * The header.
     */
    columns  = terminalWidth;
    columns -= pidFormat.realWidth();
    columns -= userFormat.realWidth();
    columns -= hostFormat.realWidth();
    columns -= priorityFormat.realWidth();
    columns -= 45;

    printf("%s", TERM_INVERSE);
    
    pidFormat.printf("PID");
    userFormat.printf("USER");
    hostFormat.printf("HOST");
    priorityFormat.printf("PR");
    printf("%s", " VIRT      RES    S   %CPU   %MEM COMMAND    ");
    
    ::printf("%s", TERM_ERASE_EOL);
    ::printf("\n\r");
    ::printf("%s", TERM_NORMAL);

    // rss          resident set size
    // vsz          virtual memory size
    for (uint idx = 0u; idx < processList.size(); ++idx)
    {
        S9sVariantMap process    = processList[idx].toVariantMap();
        int           pid        = process["pid"].toInt();
        S9sString     hostName   = process["hostname"].toString();
        S9sString     user       = process["user"].toString();
        S9sString     executable = process["executable"].toString();
        double        cpuUsage   = process["cpu_usage"].toDouble();
        double        memUsage   = process["mem_usage"].toDouble();
        S9sString     state      = process["state"].toString();
        ulonglong     rss        = process["res_mem"].toULongLong();
        ulonglong     virtMem    = process["virt_mem"].toULongLong();
        int           priority   = process["priority"].toInt();
        
        rss     /= 1024;
        virtMem /= 1024;

        pidFormat.printf(pid);
        userFormat.printf(user);
        hostFormat.printf(hostName);
        priorityFormat.printf(priority);

        printf("%8llu ", virtMem);
        printf("%8llu ", rss);
        printf("%1s ", STR(state));
        printf("%6.2f ", cpuUsage);
        printf("%6.2f ", memUsage); 
        printf("%s", STR(executable));

        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\n\r");
        ::printf("%s", TERM_NORMAL);

        if (maxLines > 0 && (int) idx + 1 >= maxLines)
            break;
    }
}

/**
 * Prints the job log in its short format. In this format only the messages are
 * printed.
 */
void
S9sRpcReply::printJobLogBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       formatString = options->briefJobLogFormat();
    bool            isDebug = options->isDebug();
    S9sVariantList  theList = operator[]("messages").toVariantList();

    if (options->hasLogFormat())
        formatString = options->logFormat();

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap  = theList[idx].toVariantMap();
        S9sMessage    message = theMap;

        if (!isDebug && message.severity() == "DEBUG")
            continue;

        if (formatString.empty())
        {
            printf("%s\n", STR(S9sString::html2ansi(message.message())));
        } else {
            printf("%s",
                    STR(message.toString(syntaxHighlight, formatString)));
        }
    }
}

/**
 * Prints the job log in its long format (aka "job --log --list").
 */
void
S9sRpcReply::printJobLogLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sString       formatString = options->longJobLogFormat();
    S9sVariantList  theList = operator[]("messages").toVariantList();
    int             terminalWidth = options->terminalWidth();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       line = S9sString::dash * terminalWidth;

    if (options->hasLogFormat())
        formatString = options->logFormat();

    /*
     * If there is a format string we simply print the list using that format.
     */
    if (!formatString.empty())
    {
        for (uint idx = 0; idx < theList.size(); ++idx)
        {
            S9sVariantMap theMap  = theList[idx].toVariantMap();
            S9sMessage    message = theMap;

            if (formatString.empty())
                printf("%s\n", STR(S9sString::html2ansi(message.message())));
            else {
                printf("%s",
                        STR(message.toString(syntaxHighlight, formatString)));
            }
        }

        return;
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap          = theList[idx].toVariantMap();
        S9sString      message         = theMap["message_text"].toString();
        S9sString      status          = theMap["message_status"].toString();
        S9sString      created         = theMap["created"].toString();
        const char    *stateColorStart = "";
        const char    *stateColorEnd   = "";
  
        message = S9sString::html2ansi(message);

        if (!created.empty())
        {
            S9sDateTime tmp;

            tmp.parse(created);
            created = tmp.toString(S9sDateTime::MySqlLogFileFormat);
        }
        
        if (status == "JOB_SUCCESS")
        {
            if (syntaxHighlight)
            {
                stateColorStart = XTERM_COLOR_GREEN;
                stateColorEnd   = TERM_NORMAL;
            }

            status = "MESSAGE";
        } else if (status == "JOB_WARNING")
        {
            if (syntaxHighlight)
            {
                stateColorStart = XTERM_COLOR_YELLOW;
                stateColorEnd   = TERM_NORMAL;
            }

            status = "WARNING";
        } else if (status == "JOB_FAILED")
        {
            if (syntaxHighlight)
            {
                stateColorStart = XTERM_COLOR_RED;
                stateColorEnd   = TERM_NORMAL;
            }

            status = "FAILURE";
        } else if (status == "JOB_DEBUG")
        {
            if (syntaxHighlight)
            {
                stateColorStart = "";
                stateColorEnd   = "";
            }

            status = "DEBUG";
        }

        //printf("%s%s%s\n\n", TERM_BOLD, STR(message), TERM_NORMAL);
        printf("%s\n\n", STR(message));

        printf("  %sCreated:%s %s%s%s  ", 
                XTERM_COLOR_DARK_GRAY, 
                TERM_NORMAL,
                XTERM_COLOR_LIGHT_GRAY,
                STR(created),
                TERM_NORMAL); 
        
        printf("%sSeverity:%s %s%s%s\n", 
                XTERM_COLOR_DARK_GRAY, 
                TERM_NORMAL,
                stateColorStart, 
                STR(status), 
                stateColorEnd); 


        printf("%s\n", STR(line));
    }
}

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
S9sRpcReply::printBackupList()
{
    S9sOptions *options = S9sOptions::instance();
    
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
    } else if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
    } else if (options->hasBackupFormat())
    {
        printBackupListFormatString(options->isLongRequested());
    } else if (options->isListFilesRequested())
    {
        // This is the normal view: the title.
        if (options->isLongRequested())
            printBackupListFilesLong();
        else
            printBackupListFilesBrief();
    } else if (options->isListDatabasesRequested())
    {
        // This is the normal view: the title.
        if (options->isLongRequested())
            printBackupListDatabasesLong();
        else
            printBackupListDatabasesBrief();
    } else {
        // This is the normal view: the title.
        if (options->isLongRequested())
            printBackupListLong();
        else
            printBackupListBrief();
    }
}

void
S9sRpcReply::printKeys()
{
    S9sOptions     *options = S9sOptions::instance();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    } else if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
        return;
    } else {
        S9sVariantList  theList = operator[]("public_keys").toVariantList();
    
        for (uint idx = 0u; idx < theList.size(); ++idx)
        {
            S9sVariantMap theMap = theList[idx].toVariantMap();

            printf("\"%s\"\n", STR(theMap["name"].toString()));
            printf("%s\n\n",   STR(theMap["key"].toString()));
        }
    
        if (!options->isBatchRequested())
            ::printf("Total: %d\n", operator[]("total").toInt());
    }
}

/**
 * Prints the account list in either the short format, the long format or the
 * JSON string format.
 */
void 
S9sRpcReply::printAccountList()
{
    S9sOptions *options = S9sOptions::instance();
    
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
        return;
    }

    if (options->isLongRequested())
        printAccountListLong();
    else
        printAccountListBrief();
}

void 
S9sRpcReply::printDatabaseList()
{
    S9sOptions *options = S9sOptions::instance();
    
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
        return;
    }

    if (options->isLongRequested())
        printDatabaseListLong();
    else
        printDatabaseListBrief();
}



/**
 * Prints the account list in short format.
 */
void 
S9sRpcReply::printAccountListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  accountList = operator[]("accounts").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    const char     *colorBegin = "";
    const char     *colorEnd   = "";
    S9sVariantMap   printed;

    /*
     * Printing the account names.
     */
    for (uint idx = 0; idx < accountList.size(); ++idx)
    {
        S9sVariantMap  accountMap   = accountList[idx].toVariantMap();
        S9sUser        account      = accountMap;
        S9sString      accountName  = account.userName();

        if (!options->isStringMatchExtraArguments(accountName))
            continue;
  
        if (printed.contains(accountName))
            continue;

        /*
         * Syntax highlight.
         */
        if (syntaxHighlight)
        {
            colorBegin = XTERM_COLOR_ORANGE;
            colorEnd   = TERM_NORMAL;
        } else {
            colorBegin = "";
            colorEnd   = "";
        }

        printf("%s%s%s\n", colorBegin, STR(accountName), colorEnd);
        printed[accountName] = true;
    }
}

/**
 * Prints the account list in long format. This should be called when the user
 * isues the "s9s account --list --long" combination.
 */
void 
S9sRpcReply::printAccountListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  accountList = operator[]("accounts").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sFormat       accountNameFormat, hostNameFormat, passwordFormat;
    S9sFormat       maxConnectionsFormat, connectionsFormat;
    int             isTerminal    = options->isTerminal();
    int             terminalWidth = options->terminalWidth();
    int             columns;
    const char     *colorBegin = "";
    const char     *colorEnd   = "";
    const char     *hostColorBegin = "";
    const char     *hostColorEnd   = "";
    
    /*
     *  Going through first and collecting some informations.
     */
    for (uint idx = 0; idx < accountList.size(); ++idx)
    {
        S9sVariantMap  accountMap   = accountList[idx].toVariantMap();
        S9sAccount     account      = accountMap;
        S9sString      accountName  = account.userName();
        S9sString      hostName     = account.hostAllow();
        S9sString      password     = account.password();
        int            maxConnections = account.maxConnections();
        int            connections  = account.connections();
        S9sString      fullName;

        if (!options->isStringMatchExtraArguments(accountName))
            continue;

        if (hostName.empty())
            hostName = "%";

        fullName.sprintf("'%s'@'%s'", STR(accountName), STR(hostName));

        if (password.empty())
            password = "N";
        else
            password = "Y";

        accountNameFormat.widen(fullName);
        hostNameFormat.widen(hostName);
        passwordFormat.widen(password);
        maxConnectionsFormat.widen(maxConnections);
        connectionsFormat.widen(connections);
    }
    
    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        accountNameFormat.widen("NAME");
        hostNameFormat.widen("HOST");
        passwordFormat.widen("P");
        connectionsFormat.widen("CONN");
        maxConnectionsFormat.widen("MAXC");

        printf("%s", headerColorBegin());
        // Johan asked for ''@'' format, this is a temporary solution for that.
        #if 0
        accountNameFormat.printf("NAME");
        hostNameFormat.printf("HOST");
        #else
        accountNameFormat.printf("NAME");
        //hostNameFormat.printf("");
        //printf(" ");
        #endif
        passwordFormat.printf("P");
        connectionsFormat.printf("CONN");
        maxConnectionsFormat.printf("MAXC");
        printf("GRANTS");
        printf("%s", headerColorEnd());

        printf("\n");
    }

    columns  = terminalWidth;
    columns -= accountNameFormat.realWidth();
    //columns -= hostNameFormat.realWidth();
    columns -= passwordFormat.realWidth();
    columns -= connectionsFormat.realWidth();
    columns -= maxConnectionsFormat.realWidth();
    columns -= 1;

    /*
     * Going through again and printing.
     */
    for (uint idx = 0; idx < accountList.size(); ++idx)
    {
        S9sVariantMap  accountMap   = accountList[idx].toVariantMap();
        S9sAccount     account      = accountMap;
        S9sString      accountName  = account.userName();
        S9sString      hostName     = account.hostAllow();
        S9sString      grants       = account.grants();
        S9sString      password     = account.password();
        int            maxConnections = account.maxConnections();
        int            connections  = account.connections();
        int            thisWidth;
        int            requiredWidth;

        if (!options->isStringMatchExtraArguments(accountName))
            continue;

        if (hostName.empty())
            hostName = "%";

        if (syntaxHighlight)
        {
            colorBegin      = XTERM_COLOR_ORANGE;
            colorEnd        = TERM_NORMAL;
            hostColorBegin  = XTERM_COLOR_GREEN;
            hostColorEnd    = TERM_NORMAL;
        }
        
        if (password.empty())
            password = "N";
        else
            password = "Y";
       
        // Johan asked for ''@'' format, this is a temporary solution for that.
        printf("%s", colorBegin);
        printf("'%s'", STR(accountName));
        printf("%s", colorEnd);
        printf("@");
        printf("%s", hostColorBegin);
        printf("'%s'", STR(hostName));
        printf("%s", hostColorEnd);
        
        // The 5 is the length of ''@''
        thisWidth = accountName.length() + hostName.length() + 5;
        requiredWidth = accountNameFormat.realWidth();

        for (int n = thisWidth; n < requiredWidth; ++n)
            printf(" ");

        passwordFormat.printf(password);
        connectionsFormat.printf(connections);
        maxConnectionsFormat.printf(maxConnections);

        if (isTerminal && (int) grants.length() > columns && columns > 1)
        {
            grants.resize(columns - 1);
            grants += "…";
        }

        printf("%s", STR(grants));
        printf("\n");
    }
    
    if (!options->isBatchRequested())
        printf("Total: %d\n", operator[]("total").toInt());
}

void
S9sRpcReply::printGroupList()
{
    S9sOptions *options = S9sOptions::instance();
    
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
        return;
    }

    if (options->isLongRequested())
        printGroupListLong();
    else
        printGroupListBrief();

}

void 
S9sRpcReply::printMaintenanceList()
{
    S9sOptions *options = S9sOptions::instance();
    
    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (options->isLongRequested())
        printMaintenanceListLong();
    else
        printMaintenanceListBrief();
}

/**
 * Generic method that prints the reply as a node list.
 */
void 
S9sRpcReply::printNodeList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (!isOk())
        PRINT_ERROR("%s", STR(errorString()));
    else if (options->isStatRequested())
        printNodeListStat();
    else if (options->isLongRequested())
        printNodeListLong();
    else
        printNodeListBrief();
}

void
S9sRpcReply::printConfigList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (options->isDebug())
        printConfigDebug();
    else if (options->isLongRequested())
        printConfigLong();
    else 
        printConfigBrief();
}

void 
S9sRpcReply::printLogList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (options->isLongRequested())
        printLogLong();
    else 
        printLogBrief();
}

void
S9sRpcReply::printLogBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    bool            isDebug = options->isDebug();
    S9sString       formatString = options->briefLogFormat();
    S9sVariantList  theList = operator[]("log_entries").toVariantList();

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap  = theList[idx].toVariantMap();
        S9sMessage    message = theMap;
        S9sString     severity = message.severity();

        if (!isDebug && (severity == "DEBUG" || severity == "INFO"))
            continue;

        if (formatString.empty())
            printf("%s\n", STR(S9sString::html2ansi(message.message())));
        else {
            printf("%s",
                    STR(message.toString(syntaxHighlight, formatString)));
        }
    }
}

void
S9sRpcReply::printLogLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    bool            isDebug = options->isDebug();
    S9sString       formatString = options->longLogFormat();
    S9sVariantList  theList = operator[]("log_entries").toVariantList();

    // FIXME:
    // The implementation of the long format is just a formatstring, the same
    // code is used here as it is added to the brief format.
    if (formatString.empty())
        formatString = "%C %36B:%-5L: %-8S %M\n";

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap  = theList[idx].toVariantMap();
        S9sMessage    message = theMap;
        S9sString     severity = message.severity();

        if (!isDebug && (severity == "DEBUG" || severity == "INFO"))
            continue;

        if (formatString.empty())
            printf("%s\n", STR(S9sString::html2ansi(message.message())));
        else {
            printf("%s",
                    STR(message.toString(syntaxHighlight, formatString)));
        }
    }
}

void
S9sRpcReply::printConfigDebug()
{
    S9sVariantList fileList = operator[]("files").toVariantList();

    for (uint idx = 0u; idx < fileList.size(); ++idx)
    {
        S9sVariantMap  fileMap  = fileList[idx].toVariantMap();
        S9sString      fileName = fileMap["filename"].toString();
        S9sString      path     = fileMap["path"].toString();
        S9sString      syntax   = fileMap["syntax"].toString();
        S9sString      content  = fileMap["content"].toString();
        S9sVariantList lines;
        
        content.replace("\\r\\n", "\n");
        content.replace("\\r", "\n");
        content.replace("\\n", "\n");

        lines = content.split("\n", true);

        ::printf("filename : %s\n", STR(fileName));
        ::printf("    path : %s\n", STR(path));
        ::printf("  syntax : %s\n", STR(syntax));

        for (uint idx1 = 0u; idx1 < lines.size(); ++idx1)
        {
            S9sString  line = lines[idx1].toString();

            //line.replace("\\r", "");
            ::printf("[%04u] %s\n", idx1, STR(line));
        }
    }
}

void
S9sRpcReply::printConfigLong()
{
    S9sOptions     *options = S9sOptions::instance();
    int             terminalWidth = options->terminalWidth();
    S9sVariant      config   = operator[]("config");
    S9sVariantList  fileList = config["files"].toVariantList();
    S9sFormat       nameFormat;
    S9sFormat       valueFormat;
    S9sFormat       sectionFormat;

    for (uint idx1 = 0; idx1 < fileList.size(); ++idx1)
    {
        S9sVariantMap  fileMap = fileList[idx1].toVariantMap();
        S9sVariantList valueList = fileMap["values"].toVariantList();

        for (uint idx2 = 0; idx2 < valueList.size(); ++idx2)
        {
            S9sVariantMap valueMap = valueList[idx2].toVariantMap();
            S9sString     section  = valueMap["section"].toString();
            S9sString     name     = valueMap["variablename"].toString();

            if (!options->optName().empty() && 
                    options->optName() != name)
            {
                continue;
            }
            
            if (!options->optGroup().empty() && 
                    options->optGroup() != section)
            {
                continue;
            }

            if (!options->isStringMatchExtraArguments(name))
                continue;

            if (section.empty())
                section = "-";

            sectionFormat.widen(section);
            valueFormat.widen(valueMap["value"].toString());
            nameFormat.widen(name);
        }
    }
        
    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        sectionFormat.widen("GROUP");
        nameFormat.widen("OPTION NAME");
        valueFormat.widen("VALUE");
        
        printf("%s", headerColorBegin());
        sectionFormat.printf("GROUP");
        nameFormat.printf("OPTION NAME");
        valueFormat.printf("VALUE");
        printf("%s", headerColorEnd());
        printf("\n");
    }

    for (uint idx1 = 0; idx1 < fileList.size(); ++idx1)
    {
        S9sVariantMap  fileMap = fileList[idx1].toVariantMap();
        S9sVariantList valueList = fileMap["values"].toVariantList();

        for (uint idx2 = 0; idx2 < valueList.size(); ++idx2)
        {
            S9sVariantMap valueMap = valueList[idx2].toVariantMap();
            S9sString     section  = valueMap["section"].toString();
            S9sString     filePath = valueMap["filepath"].toString();
            int           line     = valueMap["linenumber"].toInt(-1);
            S9sString     name     = valueMap["variablename"].toString();
            
            if (!options->optName().empty() && 
                    options->optName() != name)
            {
                continue;
            }
            
            if (!options->optGroup().empty() && 
                    options->optGroup() != section)
            {
                continue;
            }
            
            if (!options->isStringMatchExtraArguments(name))
                continue;

            if (section.empty())
                section = "-";

            printf("%sFile    :%s %s%s%s:%d\n", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_ORANGE, STR(filePath), TERM_NORMAL,
                line);
            
            printf("%sSection :%s %s\n", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                STR(section));
            
            printf("%sName    :%s %s%s%s\n", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                optNameColorBegin(), STR(name), optNameColorEnd());
            
            printf("%sValue   :%s %s\n", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                STR(valueMap["value"].toString()));
            
            // A horizontal line.
            for (int n = 0; n < terminalWidth; ++n)
                printf("-");
            printf("\n");
        }
    }
}

void
S9sRpcReply::printConfigBriefWiden(
        S9sVariantMap   map, 
        S9sFormat      &sectionFormat,
        S9sFormat      &nameFormat,
        S9sFormat      &valueFormat,
        int            depth)
{
    S9sVector<S9sString> keys = map.keys();

    for (uint idx = 0; idx < keys.size(); ++idx)
    {
        S9sString     section  = map["section"].toString();
        S9sString     name     = keys[idx];
        S9sString     value    = map[name].toString();

        if (section.empty())
            section = "-";
        
        for (int n = 0; n < depth; ++n)
            name = "    " + name;

        sectionFormat.widen(section);
        nameFormat.widen(name);
        valueFormat.widen(value);
    }
}

void
S9sRpcReply::printConfigBrief(
        S9sVariantMap   map, 
        S9sFormat      &sectionFormat,
        S9sFormat      &nameFormat,
        S9sFormat      &valueFormat,
        int            depth)
{
    S9sVector<S9sString> keys = map.keys();

    for (uint idx = 0; idx < keys.size(); ++idx)
    {
        S9sString     section  = map["section"].toString();
        S9sString     name     = keys[idx];
        S9sString     value    = map[name].toString();

        if (section.empty())
            section = "-";
        
        for (int n = 0; n < depth; ++n)
            name = "    " + name;

        sectionFormat.printf(section);

        printf("%s", optNameColorBegin());
        nameFormat.printf(name);
        printf("%s", optNameColorEnd());
        
        valueFormat.printf(value);

        printf("\n");
    }
}

void
S9sRpcReply::printConfigBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariant      config   = operator[]("config");
    S9sVariantList  fileList = config["files"].toVariantList();
    S9sFormat       nameFormat;
    S9sFormat       valueFormat;
    S9sFormat       sectionFormat;
    int             total = 0;

    for (uint idx1 = 0; idx1 < fileList.size(); ++idx1)
    {
        S9sVariantMap  fileMap = fileList[idx1].toVariantMap();
        S9sVariantList valueList = fileMap["values"].toVariantList();

        for (uint idx2 = 0; idx2 < valueList.size(); ++idx2)
        {
            S9sVariantMap valueMap = valueList[idx2].toVariantMap();
            S9sString     section  = valueMap["section"].toString();
            S9sString     name     = valueMap["variablename"].toString();
            
            if (valueMap["value"].isVariantMap())
            {
                printConfigBriefWiden(
                        valueMap["value"].toVariantMap(),
                        sectionFormat, nameFormat, valueFormat, 1);
                continue;
            }

            total++;
            if (!options->optName().empty() && 
                    options->optName() != name)
            {
                continue;
            }

            if (!options->optGroup().empty() && 
                    options->optGroup() != section)
            {
                continue;
            }

            if (!options->isStringMatchExtraArguments(name))
                continue;

            if (section.empty())
                section = "-";

            sectionFormat.widen(section);
            valueFormat.widen(valueMap["value"].toString());
            nameFormat.widen(name);
        }
    }
        
    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        sectionFormat.widen("GROUP");
        nameFormat.widen("OPTION NAME");
        valueFormat.widen("VALUE");
        
        printf("%s", headerColorBegin());
        sectionFormat.printf("GROUP");
        nameFormat.printf("OPTION NAME");
        valueFormat.printf("VALUE");
        printf("%s", headerColorEnd());
        printf("\n");
    }

    for (uint idx1 = 0; idx1 < fileList.size(); ++idx1)
    {
        S9sVariantMap  fileMap = fileList[idx1].toVariantMap();
        S9sVariantList valueList = fileMap["values"].toVariantList();

        for (uint idx2 = 0; idx2 < valueList.size(); ++idx2)
        {
            S9sVariantMap valueMap = valueList[idx2].toVariantMap();
            S9sString     section  = valueMap["section"].toString();
            S9sString     name     = valueMap["variablename"].toString();

            if (valueMap["value"].isVariantMap())
            {
                printConfigBrief(
                        valueMap["value"].toVariantMap(),
                        sectionFormat, nameFormat, valueFormat, 1);
                continue;
            }

            if (!options->optName().empty() && 
                    options->optName() != name)
            {
                continue;
            }
            
            if (!options->optGroup().empty() && 
                    options->optGroup() != section)
            {
                continue;
            }

            if (!options->isStringMatchExtraArguments(name))
                continue;

            if (section.empty())
                section = "-";
            
            sectionFormat.printf(section);

            printf("%s", optNameColorBegin());
            nameFormat.printf(name);
            printf("%s", optNameColorEnd());

            valueFormat.printf(valueMap["value"].toString());
            printf("\n");
        }
    }
    
    if (!options->isBatchRequested())
        printf("Total: %d\n", total);
}

void
S9sRpcReply::saveConfig(
        S9sString outputDir)
{
    S9sVariantList files = operator[]("files").toVariantList();
    int            nFilesExist = 0;

    /*
     * First we check if the files are not already there. If any of the files
     * are already exist we won't crate the others either.
     */
    for (uint idx = 0; idx < files.size(); ++idx)
    {
        S9sVariantMap map      = files[idx].toVariantMap();
        S9sString     fileName = map["filename"].toString();
        S9sString     path     = S9sFile::buildPath(outputDir, fileName);;
        S9sFile       file(path);

        if (!file.exists())
            continue;

        nFilesExist++;
        PRINT_ERROR("The file '%s' already exists.", STR(path));
    }

    if (nFilesExist > 0)
        return;
   
    /*
     * Then we save the files one by one.
     */
    for (uint idx = 0; idx < files.size(); ++idx)
    {
        S9sVariantMap map      = files[idx].toVariantMap();
        S9sString     fileName = map["filename"].toString();
        S9sString     content  = map["content"].toString();
        S9sString     path;
        S9sString     errorString;
        bool          success;

        path = S9sFile::buildPath(outputDir, fileName);
        //printf("-> '%s'\n", STR(path));

        success = S9sString::writeFile(path, content, errorString);
        if (!success)
        {
            PRINT_ERROR("Error saving configuration: %s", STR(errorString));
        }
    }
}

/**
 * Private low-level function to print the cluster list in the "brief" format.
 * The brief format can be used in shell scripts to create lists to walk
 * through.
 */
void 
S9sRpcReply::printClusterListBrief()
{
    S9sOptions     *options   = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       format    = options->clusterFormat();
    bool            hasFormat = options->hasClusterFormat();
    S9sVariantList  theList   = clusters();
    int             nPrinted  = 0;

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap = theList[idx].toVariantMap();
        S9sCluster    cluster(theMap);
        S9sString     clusterName = theMap["cluster_name"].toString();
        int           clusterId   = theMap["cluster_id"].toInt();
        
        //
        // Filtering.
        //
        if (options->hasClusterIdOption())
        {
            if (clusterId != options->clusterId())
                continue;
        }
        
        if (options->hasClusterNameOption())
        {
            if (clusterName != options->clusterName())
                continue;
        }

        if (!options->isStringMatchExtraArguments(clusterName))
            continue;

        //
        // The actual printing.
        //
        if (hasFormat)
        {
            printf("%s", STR(cluster.toString(syntaxHighlight, format)));
        } else {
            printf("%s%s%s ", 
                    clusterColorBegin(), 
                    STR(clusterName), 
                    clusterColorEnd());
        }


        ++nPrinted;
    }

    if (nPrinted > 0 && !hasFormat)
    {
        printf("\n");
        fflush(stdout);
    }
}

void 
S9sRpcReply::printDatabaseListBrief()
{
    S9sOptions     *options   = S9sOptions::instance();
    S9sVariantList  theList   = clusters();
    int             nPrinted  = 0;

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList databases = theMap["databases"].toVariantList();
        S9sCluster     cluster(theMap);
        S9sString      clusterName = theMap["cluster_name"].toString();
        int            clusterId   = theMap["cluster_id"].toInt();
        
        //
        // Filtering.
        //
        if (options->hasClusterIdOption())
        {
            if (clusterId != options->clusterId())
                continue;
        }
        
        if (options->hasClusterNameOption())
        {
            if (clusterName != options->clusterName())
                continue;
        }

        for (uint idx1 = 0u; idx1 < databases.size(); ++idx1)
        {
            S9sVariantMap database = databases[idx1].toVariantMap();
            S9sString     name = database["database_name"].toString();
        
            if (!options->isStringMatchExtraArguments(name))
                continue;

            printf("%s ", STR(name));
        }

        ++nPrinted;
    }

    if (nPrinted > 0)
    {
        printf("\n");
        fflush(stdout);
    }
}

void 
S9sRpcReply::printAlarmListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = alarms();
    S9sFormat       idFormat;
    S9sFormat       clusterIdFormat;
    S9sFormat       severityFormat;
    S9sFormat       typeNameFormat;
    S9sFormat       componentNameFormat;
    S9sFormat       titleFormat;
    S9sFormat       hostNameFormat;
    int             nLines = 0;
    const char     *hostColorBegin = "";
    const char     *hostColorEnd   = "";
    const char     *keyColorBegin = "";
    const char     *keyColorEnd   = "";
        
    if (syntaxHighlight)
    {
        hostColorBegin  = XTERM_COLOR_GREEN;
        hostColorEnd    = TERM_NORMAL;

        keyColorBegin   = "\033[38;5;57m";
        keyColorEnd     = TERM_NORMAL;
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap alarmMap  = theList[idx].toVariantMap();
        S9sAlarm      alarm(alarmMap);

        if (alarm.isIgnored())
            continue;

        idFormat.widen(alarm.alarmId());
        clusterIdFormat.widen(alarm.clusterId());
        componentNameFormat.widen(alarm.componentName());
        severityFormat.widen(alarm.severityName());
        typeNameFormat.widen(alarm.typeName());
        titleFormat.widen(alarm.title());
        hostNameFormat.widen(alarm.hostName());
        ++nLines;
    }

    if (!options->isNoHeaderRequested() && nLines > 0)
    {
        idFormat.widen("ID");
        clusterIdFormat.widen("CID");
        severityFormat.widen("SEVERITY");
        componentNameFormat.widen("COMPONENT");
        typeNameFormat.widen("TYPE");
        hostNameFormat.widen("HOSTNAME");
        titleFormat.widen("TITLE");
        
        printf("%s", headerColorBegin());
        idFormat.printf("ID");
        clusterIdFormat.printf("CID");
        severityFormat.printf("SEVERITY");
        componentNameFormat.printf("COMPONENT");
        typeNameFormat.printf("TYPE");
        hostNameFormat.printf("HOSTNAME");
        titleFormat.printf("TITLE");
        
        printf("%s", headerColorEnd());
        printf("\n");
    }

    

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap alarmMap  = theList[idx].toVariantMap();
        S9sAlarm      alarm(alarmMap);

        if (alarm.isIgnored())
            continue;

        idFormat.printf(alarm.alarmId());
        clusterIdFormat.printf(alarm.clusterId());

        ::printf("%s", alarm.severityColorBegin(syntaxHighlight));
        severityFormat.printf(alarm.severityName());
        ::printf("%s", alarm.severityColorEnd(syntaxHighlight));

        ::printf("%s", keyColorBegin);
        componentNameFormat.printf(alarm.componentName());
        ::printf("%s", keyColorEnd);

        ::printf("%s", keyColorBegin);
        typeNameFormat.printf(alarm.typeName());
        ::printf("%s", keyColorEnd);

        ::printf("%s", hostColorBegin);
        hostNameFormat.printf(alarm.hostName());
        ::printf("%s", hostColorEnd);

        ::printf("%s\n", STR(alarm.title()));
    }
    
    if (!options->isBatchRequested())
    {
        printf("Total: %s%lu%s alarm(s)\n", 
                numberColorBegin(),
                (unsigned long int) theList.size(),
                numberColorEnd());
    }
}

/**
 * This method will print the reply as a detailed cluster list (aka 
 * "cluster --list --long").
 */
void 
S9sRpcReply::printClusterListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = clusters();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       requestedName = options->clusterName();
    int             isTerminal    = options->isTerminal();
    int             terminalWidth = options->terminalWidth();
    S9sString       formatString  = options->longClusterFormat();
    int             nLines = 0;
    S9sFormat       idFormat;
    S9sFormat       stateFormat;
    S9sFormat       typeFormat;
    S9sFormat       ownerFormat, groupFormat;
    S9sFormat       nameFormat;

    if (options->hasClusterFormat())
        formatString = options->clusterFormat();

    /*
     * If there is a format string we simply print the list using that format.
     */
    if (!formatString.empty())
    {
        for (uint idx = 0; idx < theList.size(); ++idx)
        {
            S9sVariantMap clusterMap  = theList[idx].toVariantMap();
            S9sCluster    cluster     = S9sCluster(clusterMap); 
            S9sString     clusterName = clusterMap["cluster_name"].toString();
            int           clusterId   = clusterMap["cluster_id"].toInt();
        
            //
            // Filtering.
            //
            if (options->hasClusterIdOption())
            {
                if (clusterId != options->clusterId())
                    continue;
            }
        
            if (options->hasClusterNameOption())
            {
                if (clusterName != options->clusterName())
                    continue;
            }

            if (!options->isStringMatchExtraArguments(clusterName))
                continue;

            /*
             * Printing using the formatstring.
             */
            printf("%s", STR(cluster.toString(syntaxHighlight, formatString)));
        }

        if (!options->isBatchRequested())
            printf("Total: %lu\n", (unsigned long int) theList.size());

        return;
    }

    /*
     * First run-through: collecting some information.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap clusterMap  = theList[idx].toVariantMap();
        S9sVariantMap ownerMap    = clusterMap["owner"].toVariantMap();
        S9sString     ownerName   = ownerMap["user_name"].toString();
        S9sVariantMap groupMap    = clusterMap["group_owner"].toVariantMap();
        S9sString     groupName   = groupMap["group_name"].toString();
        S9sString     clusterName = clusterMap["cluster_name"].toString();
        int           clusterId   = clusterMap["cluster_id"].toInt();
        S9sString     clusterType = clusterMap["cluster_type"].toString();
        S9sString     state       = clusterMap["state"].toString();
        S9sString     version     = 
            clusterMap["vendor"].toString() + " " +
            clusterMap["version"].toString();
        
        //
        // Filtering.
        //
        if (options->hasClusterIdOption())
        {
            if (clusterId != options->clusterId())
                continue;
        }
        
        if (options->hasClusterNameOption())
        {
            if (clusterName != options->clusterName())
                continue;
        }

        if (!options->isStringMatchExtraArguments(clusterName))
            continue;

        if (groupName.empty())
            groupName = "0";

        idFormat.widen(clusterId);
        stateFormat.widen(state);
        typeFormat.widen(clusterType);
        ownerFormat.widen(ownerName);
        groupFormat.widen(groupName);
        nameFormat.widen(clusterName);

        ++nLines;
    }

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested() && nLines > 0)
    {
        idFormat.widen("ID");
        stateFormat.widen("STATE");
        typeFormat.widen("TYPE");
        ownerFormat.widen("OWNER");
        groupFormat.widen("GROUP");
        nameFormat.widen("NAME");

        printf("%s", headerColorBegin());
        idFormat.printf("ID");
        stateFormat.printf("STATE");
        typeFormat.printf("TYPE");
        ownerFormat.printf("OWNER");
        groupFormat.printf("GROUP");
        nameFormat.printf("NAME");
        printf("COMMENT");
        
        printf("%s", headerColorEnd());
        printf("\n");
    }

    /*
     * Second run: doing the actual printing.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap clusterMap  = theList[idx].toVariantMap();
        S9sVariantMap ownerMap    = clusterMap["owner"].toVariantMap();
        S9sString     ownerName   = ownerMap["user_name"].toString();
        S9sVariantMap groupMap    = clusterMap["group_owner"].toVariantMap();
        S9sString     groupName   = groupMap["group_name"].toString();
        S9sString     clusterName = clusterMap["cluster_name"].toString();
        int           clusterId   = clusterMap["cluster_id"].toInt();
        S9sString     clusterType = clusterMap["cluster_type"].toString();
        S9sString     state       = clusterMap["state"].toString();
        S9sString     statusText  = clusterMap["status_text"].toString();
        int           nColumns    = 0;
        S9sString     version     = 
            clusterMap["vendor"].toString() + " " +
            clusterMap["version"].toString();
        
        if (groupName.empty())
            groupName = "0";

        //
        // Filtering
        //
        if (options->hasClusterIdOption())
        {
            if (clusterId != options->clusterId())
                continue;
        }

        if (options->hasClusterNameOption())
        {
            if (clusterName != options->clusterName())
                continue;
        }

        if (!requestedName.empty() && requestedName != clusterName)
            continue;
        
        if (!options->isStringMatchExtraArguments(clusterName))
            continue;

        // This should not happen!
        if (clusterName.empty())
            continue;
        
        nColumns += idFormat.realWidth();
        nColumns += stateFormat.realWidth();
        nColumns += typeFormat.realWidth();
        nColumns += ownerFormat.realWidth();
        nColumns += groupFormat.realWidth();
        nColumns += nameFormat.realWidth();

        if (isTerminal && nColumns < terminalWidth)
        {
            int remaining  = terminalWidth - nColumns;
            
            if (remaining < (int) statusText.length())
            {
                statusText.resize(remaining - 1);
                statusText += "…";
            }
        }
        
        if (syntaxHighlight)
        {
            if (state == "STARTED")
                stateFormat.setColor(XTERM_COLOR_GREEN, TERM_NORMAL);
            else if (state == "FAILED" || state == "FAILURE")
                stateFormat.setColor(XTERM_COLOR_RED, TERM_NORMAL);
            else
                stateFormat.setColor(XTERM_COLOR_YELLOW, TERM_NORMAL);
        }

        idFormat.printf(clusterId); 
        stateFormat.printf(state);
        typeFormat.printf(clusterType.toLower());
        
        printf("%s", userColorBegin());
        ownerFormat.printf(ownerName);
        printf("%s", userColorEnd());

        printf("%s", groupColorBegin(groupName));
        groupFormat.printf(groupName);
        printf("%s", groupColorEnd());
        
        printf("%s", clusterColorBegin());
        nameFormat.printf(clusterName);
        printf("%s", clusterColorEnd());

        printf("%s\n", STR(statusText));
    }
   
    if (!options->isBatchRequested())
        printf("Total: %lu\n", (unsigned long int) theList.size());
}

void
S9sRpcReply::printAlarmList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (!isOk())
        PRINT_ERROR("%s", STR(errorString()));
    //else if (options->isStatRequested())
    //    printClusterListStat();
    else if (options->isLongRequested())
        printAlarmListLong();
    //else
    //    printClusterListBrief();    
}

/**
 * Prints the database list in long format as in 
 * s9s cluster --list-databases --long
 *
 */
void 
S9sRpcReply::printDatabaseListLong()
{ 
    S9sOptions     *options   = S9sOptions::instance();
    S9sVariantList  theList   = clusters();
    S9sFormat       sizeFormat;
    S9sFormat       nTablesFormat;
    S9sFormat       nRowsFormat;
    S9sFormat       clusterNameFormat;
    S9sFormat       nameFormat;
    S9sFormat       ownerFormat, groupFormat;
    ulonglong       totalBytes = 0ull;
    ulonglong       totalTables = 0ull;
    int             nDatabases = 0;

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList databases = theMap["databases"].toVariantList();
        S9sCluster     cluster(theMap);
        S9sString      clusterName = theMap["cluster_name"].toString();
        int            clusterId   = theMap["cluster_id"].toInt();

        //
        // Filtering.
        //
        if (options->hasClusterIdOption())
        {
            if (clusterId != options->clusterId())
                continue;
        }
        
        if (options->hasClusterNameOption())
        {
            if (clusterName != options->clusterName())
                continue;
        }

        for (uint idx1 = 0u; idx1 < databases.size(); ++idx1)
        {
            S9sVariantMap database = databases[idx1].toVariantMap();
            S9sString     name = database["database_name"].toString();
            ulonglong     size = database["database_size"].toULongLong();
            S9sString     sizeStr = m_formatter.mBytesToHuman(size / (1024*1024));
            ulonglong     nTables = database["number_of_tables"].toULongLong();
            S9sString     ownerName = database["owner_user_name"].toString();
            S9sString     groupName = database["owner_group_name"].toString();
            S9sString     nTablesString;
            S9sString     nRowsString;

            if (name == "mysql" || name == "performance_schema")
                continue;

            if (!options->isStringMatchExtraArguments(name))
                continue;

            if (database.contains("number_of_rows"))
            {
                nRowsString.sprintf(
                        "%'llu", 
                        database["number_of_rows"].toULongLong());
            } else {
                nRowsString = "-";
            }

            nTablesString.sprintf("%'llu", nTables);

            ownerFormat.widen(ownerName);
            groupFormat.widen(groupName);
            sizeFormat.widen(sizeStr);
            nTablesFormat.widen(nTablesString);
            nRowsFormat.widen(nRowsString);
            clusterNameFormat.widen(clusterName);
            nameFormat.widen(name);

        }
    }
    
    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        sizeFormat.widen("SIZE");
        nTablesFormat.widen("#TBL");
        nRowsFormat.widen("#ROWS");
        ownerFormat.widen("OWNER");
        groupFormat.widen("GROUP");
        clusterNameFormat.widen("CLUSTER");
        nameFormat.widen("DATABASE");

        printf("%s", headerColorBegin());

        sizeFormat.printf("SIZE");
        nTablesFormat.printf("#TBL");
        nRowsFormat.printf("#ROWS");
        ownerFormat.printf("OWNER");
        groupFormat.printf("GROUP");
        clusterNameFormat.printf("CLUSTER");
        nameFormat.printf("DATABASE");
 
        printf("%s", headerColorEnd());
        printf("\n");
    }

    sizeFormat.setRightJustify();
    nTablesFormat.setRightJustify();
    nRowsFormat.setRightJustify();
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList databases = theMap["databases"].toVariantList();
        S9sCluster     cluster(theMap);
        S9sString      clusterName = theMap["cluster_name"].toString();
        int            clusterId   = theMap["cluster_id"].toInt();
        
        //
        // Filtering.
        //
        if (options->hasClusterIdOption())
        {
            if (clusterId != options->clusterId())
                continue;
        }
        
        if (options->hasClusterNameOption())
        {
            if (clusterName != options->clusterName())
                continue;
        }

        for (uint idx1 = 0u; idx1 < databases.size(); ++idx1)
        {
            S9sVariantMap database = databases[idx1].toVariantMap();
            S9sString     name = database["database_name"].toString();
            ulonglong     size = database["database_size"].toULongLong();
            S9sString     sizeStr = m_formatter.mBytesToHuman(size / (1024*1024));
            ulonglong     nTables = database["number_of_tables"].toULongLong();
            S9sString     ownerName = database["owner_user_name"].toString();
            S9sString     groupName = database["owner_group_name"].toString();
            S9sString     nTablesString;
            S9sString     nRowsString;

            if (name == "mysql" || name == "performance_schema")
                continue;

            if (!options->isStringMatchExtraArguments(name))
                continue;

            if (database.contains("number_of_rows"))
            {
                nRowsString.sprintf(
                        "%'llu", 
                        database["number_of_rows"].toULongLong());
            } else {
                nRowsString = "-";
            }

            nDatabases  += 1;
            totalBytes  += size;
            totalTables += nTables;

            nTablesString.sprintf("%'llu", nTables);

            sizeFormat.printf(sizeStr);
            nTablesFormat.printf(nTablesString);
            nRowsFormat.printf(nRowsString);

            printf("%s", userColorBegin());
            ownerFormat.printf(ownerName);
            printf("%s", userColorEnd());

            printf("%s", groupColorBegin(groupName));
            groupFormat.printf(groupName);
            printf("%s", groupColorEnd());

            printf("%s", clusterColorBegin());
            clusterNameFormat.printf(clusterName);
            printf("%s", clusterColorEnd());

            printf("%s", databaseColorBegin());
            nameFormat.printf(name);
            printf("%s", databaseColorEnd());

            printf("\n");
        }
    }

    if (!options->isBatchRequested())
    {
        printf("Total: %s%d%s databases, %s%s%s, %s%'llu%s tables.\n", 
                numberColorBegin(), nDatabases, numberColorEnd(),
                numberColorBegin(),
                STR(m_formatter.mBytesToHuman(totalBytes / (1024*1024))),
                numberColorEnd(),
                numberColorBegin(), totalTables, numberColorEnd());
    }
}

/**
 * Prints the node list from the reply if the reply contains a node list. The
 * printed list is in "brief" format.
 */
void 
S9sRpcReply::printNodeListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   properties = options->propertiesOption();
    S9sVariantList  theList = clusters();
    S9sString       formatString = options->shortNodeFormat();
    int             isTerminal    = options->isTerminal();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       clusterNameFilter = options->clusterName();
    uint            maxHostNameLength = 0u;
    S9sString       hostNameFormat;
    int             terminalWidth = options->terminalWidth();
    int             nColumns;
    int             column = 0;
        
    if (options->hasNodeFormat())
        formatString = options->nodeFormat();

    /*
     * If there is a format string we simply print the list using that format.
     */
    if (!formatString.empty())
    {
        for (uint idx = 0; idx < theList.size(); ++idx)
        {
            S9sVariantMap  theMap      = theList[idx].toVariantMap();
            S9sVariantList hosts       = theMap["hosts"].toVariantList();
            S9sString      clusterName = theMap["cluster_name"].toString();

            if (!clusterNameFilter.empty() && clusterNameFilter != clusterName)
                continue;

            for (uint idx2 = 0; idx2 < hosts.size(); ++idx2)
            {
                S9sVariantMap hostMap   = hosts[idx2].toVariantMap();
                S9sNode       node      = hostMap;
                int           clusterId = node.clusterId();
                S9sCluster    cluster   = clusterMap(clusterId);
                S9sString     hostName  = node.name();
                
                // Filtering...
                if (!properties.isSubSet(hostMap))
                    continue;

                if (!options->isStringMatchExtraArguments(hostName))
                    continue;

                node.setCluster(cluster);

                printf("%s", STR(node.toString(syntaxHighlight, formatString)));
            }
        }
    
        return;
    }


    /*
     * First run: some data collecting.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList hostList = theMap["hosts"].toVariantList();

        for (uint idx2 = 0; idx2 < hostList.size(); ++idx2)
        {
            S9sVariantMap hostMap   = hostList[idx2].toVariantMap();
            S9sNode       node      = hostMap;
            S9sString     hostName  = node.name();
            S9sString     version   = hostMap["version"].toString();
                
            if (!properties.isSubSet(hostMap))
                continue;
            
            if (hostName.length() > maxHostNameLength)
                maxHostNameLength = hostName.length();
        }
    }

    nColumns          = terminalWidth / (maxHostNameLength + 1);

    if (nColumns > 0)
        maxHostNameLength = terminalWidth / nColumns;
    else 
        maxHostNameLength = 16;

    hostNameFormat.sprintf("%%s%%-%us%%s", maxHostNameLength);

    /*
     * Second run: actual printing.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sString      clusterName = theMap["cluster_name"].toString();
        S9sVariantList hostList = theMap["hosts"].toVariantList();

        for (uint idx2 = 0; idx2 < hostList.size(); ++idx2)
        {
            S9sVariantMap hostMap   = hostList[idx2].toVariantMap();
            S9sNode       node      = hostMap;
            S9sString     hostName  = node.name();
            S9sString     status    = hostMap["hoststatus"].toString();
            const char   *nameStart = "";
            const char   *nameEnd   = "";

            if (!properties.isSubSet(hostMap))
                continue;

            if (syntaxHighlight)
            {
                if (status == "CmonHostRecovery")
                {
                    nameStart = XTERM_COLOR_YELLOW;
                    nameEnd   = TERM_NORMAL;
                } else if (status == "CmonHostUnknown" ||
                        status == "CmonHostOffLine" ||
                        status == "CmonHostShutDown")
                {
                    nameStart = XTERM_COLOR_RED;
                    nameEnd   = TERM_NORMAL;
                } else {
                    nameStart = XTERM_COLOR_GREEN;
                    nameEnd   = TERM_NORMAL;
                }
            }
                   

            if (isTerminal)
            {
                printf(STR(hostNameFormat), 
                        nameStart, STR(hostName), nameEnd);

                column += maxHostNameLength;
                if (column + (int) maxHostNameLength > terminalWidth)
                {
                    printf("\n");
                    column = 0;
                }
            } else {
                printf("%s%s%s\n", nameStart, STR(hostName), nameEnd);
                column = 0;
            }
        }
    }

    if (column > 0)
    {
        printf("\n");
        fflush(stdout);
    }
}

void
S9sRpcReply::printScriptTree()
{
    S9sOptions *options = S9sOptions::instance();
    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (!isOk())
        PRINT_ERROR("%s", STR(errorString()));
    else 
        printScriptTreeBrief();
}

void 
S9sRpcReply::printScriptTreeBrief(
        S9sVariantMap        entry,
        int                  recursionLevel,
        S9sString            indentString,
        bool                 isLast)
{
    S9sString       name    = entry["name"].toString();
    S9sVariantList  entries = entry["contents"].toVariantList();
    S9sString       type    = entry["type"].toString();
    bool            isDir   = type == "directory";
    S9sString       indent;

    printf("%s", STR(indentString));

    if (recursionLevel)
    {
        if (isLast)
            indent = "└── ";
        else 
            indent = "├── ";
    }

    if (isDir)
    {
        printf("%s%s%s%s\n", 
                STR(indent), 
                XTERM_COLOR_BLUE, STR(name), TERM_NORMAL);
    } else {
        printf("%s%s%s%s\n", 
                STR(indent), 
                XTERM_COLOR_GREEN, STR(name), TERM_NORMAL);
    }

    for (uint idx = 0; idx < entries.size(); ++idx)
    {
        S9sVariantMap child = entries[idx].toVariantMap();
        bool          last = idx + 1 >= entries.size();
       
        if (recursionLevel)
        {
            if (isLast)
                indent = "    ";
            else
                indent = "│   ";
        }

        printScriptTreeBrief(
                child, recursionLevel + 1, 
                indentString + indent, last);
    }
}

void
S9sRpcReply::printScriptTreeBrief()
{
    S9sVariantMap entry =  operator[]("data").toVariantMap();

    printScriptTreeBrief(entry, 0, "", false);
}

/**
 *
 * \code{.js}
 * "processors": [ 
 * {
 *     "cores": 4,
 *     "cpu_max_ghz": 2,
 *     "id": 400,
 *     "model": "Intel(R) Xeon(R) CPU E5345 @ 2.33GHz",
 *     "siblings": 4,
 *     "vendor": "Intel Corp."
 * }, 
 * {
 *     "cores": 4,
 *     "cpu_max_ghz": 2,
 *     "id": 401,
 *     "model": "Intel(R) Xeon(R) CPU E5345 @ 2.33GHz",
 *     "siblings": 4,
 *     "vendor": "Intel Corp."
 * } ],
 * \endcode
 */
void 
S9sRpcReply::printProcessors(
        S9sString indent)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    int             totalCpus = 0;
    int             totalCores = 0;
    int             totalThreads = 0;
    bool            compact = !options->isLongRequested();
    S9sVariantMap   cpuModels;
    
    if (!options->isNoHeaderRequested() && compact)
    {
        printf("%s", headerColorBegin());
        printf("QUA   PROCESSOR\n");
        printf("%s", headerColorEnd());    
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList processorList = theMap["processors"].toVariantList();
        S9sString      hostName = theMap["hostname"].toString();
 
        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        if (!compact)
        {
            S9sString name = theMap["hostname"].toString();
            S9sString model = theMap["model"].toString();

            printf("%s (%s)\n", STR(name), STR(model));
        }

        for (uint idx1 = 0; idx1 < processorList.size(); ++idx1)
        {
            S9sVariantMap processor = processorList[idx1].toVariantMap();
            S9sString     model     = processor["model"].toString();
            int           cores     = processor["cores"].toInt();
            int           threads   = processor["siblings"].toInt();

            if (compact)
            {
                cpuModels[model] += 1;
            } else {
                printf("    %s", STR(indent));
                printf("%s", STR(model));
                printf("\n");
            }
                
            ++totalCpus;
            totalCores += cores;
            totalThreads += threads;
        }
    }

    if (compact)
    {
        for (uint idx = 0; idx < cpuModels.keys().size(); ++idx)
        {
            S9sString  name = cpuModels.keys().at(idx);
            int        volume = cpuModels[name].toInt();

            printf("%s", STR(indent));
            printf("%3d x %s\n", volume, STR(name));
        }
    }

    if (!options->isBatchRequested())
    {
        printf("%sTotal: %s%d%s cpus, %s%d%s cores, %s%d%s threads\n", 
                STR(indent),
                numberColorBegin(), totalCpus, numberColorEnd(),
                numberColorBegin(), totalCores, numberColorEnd(),
                numberColorBegin(), totalThreads, numberColorEnd());
    }
}

void 
S9sRpcReply::printDisks(
        S9sString indent)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    int             totalDisks = 0;
    bool            compact = !options->isLongRequested();
    S9sVariantMap   diskModels;
    ulonglong       totalCapacity = 0;

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList processorList = theMap["disk_devices"].toVariantList();
        S9sString      hostName = theMap["hostname"].toString();
        
        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        if (!compact)
        {
            S9sString name = theMap["hostname"].toString();
            S9sString model = theMap["model"].toString();

            printf("%s (%s)\n", STR(name), STR(model));
        }

        for (uint idx1 = 0; idx1 < processorList.size(); ++idx1)
        {
            S9sVariantMap processor = processorList[idx1].toVariantMap();
            S9sString     model     = processor["model"].toString();
            bool isHw = processor["is_hardware_storage"].toBoolean();
            ulonglong     capacity  = processor["total_mb"].toULongLong();

            if (!isHw)
            {
                continue;
            }

            //capacity *= 1024 * 1024;
            //capacity /= 1000000000ull;
            if (capacity > 0ull)
            {
                capacity /= 1024;
                totalCapacity += capacity;
                model.sprintf("%llu GBytes %s", capacity, STR(model));
            }

            if (compact)
            {
                diskModels[model] += 1;
            } else {
                printf("    %s", STR(indent));
                printf("%s", STR(model));
                printf("\n");
            }
                
            ++totalDisks;
        }
    }

    if (compact)
    {
        for (uint idx = 0; idx < diskModels.keys().size(); ++idx)
        {
            S9sString  name = diskModels.keys().at(idx);
            int        volume = diskModels[name].toInt();

            printf("%s", STR(indent));
            printf("%3d x %s\n", volume, STR(name));
        }
    }

    printf("%sTotal: %d disks, %llu GBytes\n", 
            STR(indent), totalDisks, totalCapacity);
}

void 
S9sRpcReply::printNics(
        S9sString indent)
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    int             totalDisks = 0;
    int             totalLink  = 0;
    bool            compact = !options->isLongRequested();
    S9sVariantMap   diskModels;
    
    if (!options->isNoHeaderRequested() && compact)
    {
        printf("%s", headerColorBegin());
        printf("QUA   NIC\n");
        printf("%s", headerColorEnd());
        
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList nicList = theMap["network_interfaces"].toVariantList();
        S9sString      hostName = theMap["hostname"].toString();

        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        if (!compact)
        {
            S9sString name = theMap["hostname"].toString();
            S9sString model = theMap["model"].toString();

            printf(TERM_BOLD);
            printf("%s (%s)\n", STR(name), STR(model));
            printf(TERM_NORMAL);
        }

        for (uint idx1 = 0; idx1 < nicList.size(); ++idx1)
        {
            S9sVariantMap processor = nicList[idx1].toVariantMap();
            S9sString     model     = processor["model"].toString();
            S9sString     mac       = processor["mac"].toString();
            bool          link      = processor["link"].toBoolean();
            
            if (model.empty())
                continue;

            if (compact)
            {
                diskModels[model] += 1;
            } else {
                printf("    %s", STR(indent));

                if (link)
                    printf(XTERM_COLOR_NIC_UP);
                else
                    printf(XTERM_COLOR_NIC_NOLINK);

                printf("%s ", STR(mac));
                printf("%s", STR(model));

                printf(TERM_NORMAL);

                printf("\n");
            }
                
            ++totalDisks;

            if (link)
                totalLink += 1;
        }
    }

    if (compact)
    {
        for (uint idx = 0; idx < diskModels.keys().size(); ++idx)
        {
            S9sString  name = diskModels.keys().at(idx);
            int        volume = diskModels[name].toInt();

            printf("%s", STR(indent));
            printf("%3d x %s\n", volume, STR(name));
        }
    }

    if (syntaxHighlight)
    {
        printf("%sTotal: %s%d%s network interfaces, %s%d%s connected\n", 
            STR(indent), 
            TERM_BOLD, totalDisks, TERM_NORMAL, 
            TERM_BOLD, totalLink, TERM_NORMAL);
    } else {
        printf("%sTotal: %d network interfaces, %d connected\n", 
            STR(indent), 
            totalDisks, 
            totalLink);
    }
}

void 
S9sRpcReply::printPartitions(
        S9sString indent)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    S9sFormat       totalFormat;
    S9sFormat       freeFormat;
    S9sFormat       usedFormat;
    S9sFormat       percentFormat;
    S9sFormat       hostnameFormat;
    S9sFormat       filesystemFormat;
    S9sFormat       deviceFormat;
    S9sFormat       mountpointFormat;
    ulonglong       totalTotal = 0ull;
    ulonglong       freeTotal = 0ull;

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList deviceList = theMap["disk_devices"].toVariantList();
        S9sString      hostName   = theMap["hostname"].toString();
        
        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        hostnameFormat.widen(hostName);

        for (uint idx1 = 0; idx1 < deviceList.size(); ++idx1)
        {
            S9sVariantMap device = deviceList[idx1].toVariantMap();
            S9sString     className = device["class_name"].toString();
            S9sString     deviceName = device["device"].toString();
            S9sString     filesystem = device["filesystem"].toString();
            ulonglong     total = device["total_mb"].toULongLong();
            S9sString     totalStr = m_formatter.mBytesToHuman(total);
            ulonglong     free = device["free_mb"].toULongLong();
            S9sString     freeStr = m_formatter.mBytesToHuman(free);
            S9sString     usedStr = m_formatter.mBytesToHuman(total - free);
            S9sString     percentStr;

            if (total == 0ull)
                continue;

            if (className != "CmonDiskDevice")
                continue;

            percentStr.sprintf("%.1f%%", 100.0 * (total - free) / total);

            totalFormat.widen(totalStr);
            usedFormat.widen(usedStr);
            freeFormat.widen(freeStr);
            percentFormat.widen(percentStr);

            filesystemFormat.widen(filesystem);
            deviceFormat.widen(deviceName);
        }
    }

    totalFormat.setRightJustify();
    usedFormat.setRightJustify();
    freeFormat.setRightJustify();
    percentFormat.setRightJustify();

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        totalFormat.widen("SIZE");
        usedFormat.widen("USED");
        freeFormat.widen("AVAIL");
        percentFormat.widen("USE%");
        hostnameFormat.widen("HOST");
        filesystemFormat.widen("FS");
        deviceFormat.widen("DEVICE");
        mountpointFormat.widen("MOUNT POINT");

        printf("%s", headerColorBegin());
        totalFormat.printf("SIZE");
        usedFormat.printf("USED");
        freeFormat.printf("AVAIL");
        percentFormat.printf("USE%");

        hostnameFormat.printf("HOST");
        filesystemFormat.printf("FS");
        deviceFormat.printf("DEVICE");
        mountpointFormat.printf("MOUNT POINT");
        printf("%s", headerColorEnd());

        printf("\n");
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList deviceList = theMap["disk_devices"].toVariantList();
        S9sString      hostName   = theMap["hostname"].toString();

        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        for (uint idx1 = 0; idx1 < deviceList.size(); ++idx1)
        {
            S9sVariantMap device = deviceList[idx1].toVariantMap();
            S9sString     className = device["class_name"].toString();
            S9sString     deviceName = device["device"].toString();
            S9sString     filesystem = device["filesystem"].toString();
            ulonglong     total = device["total_mb"].toULongLong();
            S9sString     totalStr = m_formatter.mBytesToHuman(total);
            ulonglong     free = device["free_mb"].toULongLong();
            S9sString     freeStr = m_formatter.mBytesToHuman(free);
            S9sString     usedStr = m_formatter.mBytesToHuman(total - free);
            S9sString     percentStr;
            S9sString     mountPoint = device["mountpoint"].toString();

            if (className != "CmonDiskDevice")
                continue;
            
            if (total == 0ull)
                continue;

            percentStr.sprintf("%.1f%%", 100.0 * (total - free) / total);

            totalTotal += total;
            freeTotal  += free;

            totalFormat.printf(totalStr);
            usedFormat.printf(usedStr);
            freeFormat.printf(freeStr);
            percentFormat.printf(percentStr);
            
            printf("%s", serverColorBegin());
            hostnameFormat.printf(hostName);
            printf("%s", serverColorEnd());

            printf("%s", XTERM_COLOR_FILESYSTEM);
            filesystemFormat.printf(filesystem);
            printf("%s", TERM_NORMAL);
            
            printf("%s", XTERM_COLOR_BDEV);
            deviceFormat.printf(deviceName);
            printf("%s", TERM_NORMAL);

            printf("%s", XTERM_COLOR_DIR);
            printf("%s", STR(mountPoint));
            printf("%s", TERM_NORMAL);

            printf("\n");
        }
    }

    if (!options->isBatchRequested())
    {
        printf("Total: %s%s%s, %s%s%s free\n", 
                numberColorBegin(), 
                STR(m_formatter.mBytesToHuman(totalTotal)),
                numberColorEnd(),
                numberColorBegin(), 
                STR(m_formatter.mBytesToHuman(freeTotal)), 
                numberColorEnd());
    } 
}

/**
 *
 * \code{.js}
 * "memory": 
 * {
 *     "banks": [ 
 *     {
 *         "bank": "0",
 *         "name": "DIMM 800 MHz (1.2 ns)",
 *         "size": 4294967296
 *     } ],
 *     "class_name": "CmonMemoryInfo",
 *     "memory_available_mb": 61175,
 *     "memory_free_mb": 54846,
 *     "memory_total_mb": 64421,
 *     "swap_free_mb": 0,
 *     "swap_total_mb": 0
 * },
 * \endcode
 */
void 
S9sRpcReply::printMemoryBanks(
        S9sString indent)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    int             totalModules = 0;
    ulonglong       totalSize = 0ull;
    ulonglong       freeSize  = 0ull;
    bool            compact = !options->isLongRequested();
    S9sVariantMap   memoryModels;

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested() && compact)
    {
        printf("%s", headerColorBegin());
        printf("QUA   MODULE\n");
        printf("%s", headerColorEnd()); 
    }

    /*
     * Going through the list and printing the modules. If this is a compact
     * view we only collect data here, print later.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantMap  memory = theMap["memory"].toVariantMap();
        S9sVariantList processorList = memory["banks"].toVariantList();
        int            totalOnServer = memory["memory_total_mb"].toInt();
        int            freeOnServer  = memory["memory_free_mb"].toInt();
        S9sString      hostName = theMap["hostname"].toString();
 
        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        totalSize += totalOnServer;
        freeSize  += freeOnServer;

        if (!compact)
        {
            S9sString name = theMap["hostname"].toString();
            S9sString model = theMap["model"].toString();

            printf("%s (%s)\n", STR(name), STR(model));
        }

        for (uint idx1 = 0; idx1 < processorList.size(); ++idx1)
        {
            S9sVariantMap processor = processorList[idx1].toVariantMap();
            S9sString     model     = processor["name"].toString();
            ulonglong     size      = processor["size"].toULongLong();
            int           gBytes    = size / (1024ull * 1024ull * 1024ull);

            model.sprintf("%d Gbyte %s", gBytes, STR(model));

            if (compact)
            {
                memoryModels[model] += 1;
            } else {
                printf("    %s", STR(indent));
                printf("%s", STR(model));
                printf("\n");
            }
                
            ++totalModules;
        }
    }

    /*
     * If this is a compact list we print here.
     */
    if (compact)
    {
        for (uint idx = 0; idx < memoryModels.keys().size(); ++idx)
        {
            S9sString  name   = memoryModels.keys().at(idx);
            int        volume = memoryModels[name].toInt();

            printf("%s", STR(indent));
            printf("%3d x %s\n", volume, STR(name));
        }
    }

    /*
     * Printing the total.
     */
    if (!options->isBatchRequested())
    {
        printf("%s", STR(indent));

        printf(
            "Total: %s%d%s modules, %s%d%s GBytes, %s%d%s GBytes free\n", 
            numberColorBegin(), totalModules, numberColorEnd(),
            numberColorBegin(), (int)(totalSize/1024), numberColorEnd(), 
            numberColorBegin(), (int)(freeSize/1024), numberColorEnd());
    }
}

// Deprecated.
void
S9sRpcReply::printRegionsBrief()
{
    //S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("servers").toVariantList();

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap     = theList[idx].toVariantMap();
        S9sVariantList regionList = theMap["regions_supported"].toVariantList();

        for (uint idx1 = 0; idx1 < regionList.size(); ++idx1)
        {
            S9sString regionName = regionList[idx1].toString();
            ::printf("%s\n", STR(regionName));
        }
    }
}

void
S9sRpcReply::printTemplates()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    S9sFormat       cloudFormat;
    S9sFormat       regionFormat;
    S9sFormat       cpuFormat, memoryFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       nameFormat;
    S9sFormat       vpcFormat;
    S9sFormat       idFormat;
    int             nTemplatesFound = 0;
    S9sStringList   regions;
    
    // If the json is requested we simply print it and that's all.
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap   = theList[idx].toVariantMap();
        S9sServer      server   = theMap;
        S9sString      hostName = server.hostName();
        int            nTemplates = server.nTemplates();

        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        for (int idx1 = 0; idx1 < nTemplates; ++idx1)
        {
            S9sString cloud  = server.templateProvider(idx1);
            S9sString region = server.templateRegion(idx1, "all");
            S9sString name   = server.templateName(idx1, true);
            int       nCpus  = server.templatenVcpus(idx1);
            S9sString memory = server.templateMemory(idx1, "-");

            if (!options->cloudName().empty() && 
                    options->cloudName() != cloud)
            {
                continue;
            }

            cloudFormat.widen(cloud);
            regionFormat.widen(region);
            cpuFormat.widen(nCpus);
            memoryFormat.widen(memory);
            hostNameFormat.widen(hostName);
            nameFormat.widen(name);

            if (!regions.contains(region))
                regions << region;

            ++nTemplatesFound;
        }
    }
    
    if (!options->isNoHeaderRequested())
    {
        cloudFormat.widen("CLD");
        regionFormat.widen("REGION");
        cpuFormat.widen("CPU");
        memoryFormat.widen("MEMORY");
        hostNameFormat.widen("SERVER");
        nameFormat.widen("TEMPLATE");

        printf("%s", headerColorBegin());
        cloudFormat.printf("CLD");
        regionFormat.printf("REGION");
        cpuFormat.printf("CPU");
        memoryFormat.printf("MEMORY");
        hostNameFormat.printf("SERVER");
        nameFormat.printf("TEMPLATE", false);
        printf("%s", headerColorEnd());
        printf("\n");

    }
    
    cpuFormat.setRightJustify();
    memoryFormat.setRightJustify();
    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap   = theList[idx].toVariantMap();
        S9sServer      server   = theMap;
        S9sString      hostName = server.hostName();
        int            nTemplates = server.nTemplates();

        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        for (int idx1 = 0; idx1 < nTemplates; ++idx1)
        {
            S9sString cloud  = server.templateProvider(idx1);
            S9sString region = server.templateRegion(idx1, "all");
            S9sString name   = server.templateName(idx1, true);
            int       nCpus  = server.templatenVcpus(idx1);
            S9sString memory = server.templateMemory(idx1, "-");

            if (!options->cloudName().empty() && 
                    options->cloudName() != cloud)
            {
                continue;
            }

            hostNameFormat.setColor(
                    server.colorBegin(syntaxHighlight),
                    server.colorEnd(syntaxHighlight));
            
            cloudFormat.printf(cloud);
            regionFormat.printf(region);
            if (nCpus > 0)
                cpuFormat.printf(nCpus);
            else
                cpuFormat.printf("-");

            memoryFormat.printf(memory);
            hostNameFormat.printf(hostName);
            nameFormat.printf(name);

            ::printf("\n");
        }
    }
    
    if (!options->isBatchRequested())
    {
        ::printf("Total %s%d%s template(s) in %s%d%s region(s).\n",
                numberColorBegin(), nTemplatesFound, numberColorEnd(),
                numberColorBegin(), (int)regions.size(), numberColorEnd()
                );
    }
}

void
S9sRpcReply::printSubnets()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    S9sFormat       cloudFormat;
    S9sFormat       regionFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       cidrFormat(ipColorBegin(), ipColorEnd());
    S9sFormat       vpcFormat;
    S9sFormat       idFormat;
    int             nSubnetsFound = 0;
    S9sStringList   regions;
   
    // If the json is requested we simply print it and that's all.
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    // If no --long option is provided we print a simple list.
    if (!options->isLongRequested())
    {
        S9sVariantMap subnetsMap;

        for (uint idx = 0; idx < theList.size(); ++idx)
        {
            S9sVariantMap  theMap   = theList[idx].toVariantMap();
            S9sServer      server   = theMap;
            S9sString      hostName = server.hostName();
            int            nSubnets = server.nSubnets();

            for (int idx1 = 0; idx1 < nSubnets; ++idx1)
            {
                S9sString cloud  = server.subnetProvider(idx1);
                S9sString id     = server.subnetId(idx1);

                subnetsMap[id] = id;
            }
        }

        foreach(S9sVariant name, subnetsMap)
        {
            ::printf("%s ", STR(name.toString()));
        }

        ::printf("\n");
        return;
    }

    /*
     * Collecting information for the long variant of the list.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap   = theList[idx].toVariantMap();
        S9sServer      server   = theMap;
        S9sString      hostName = server.hostName();
        int            nSubnets = server.nSubnets();

        for (int idx1 = 0; idx1 < nSubnets; ++idx1)
        {
            S9sString cloud  = server.subnetProvider(idx1);
            S9sString region = server.subnetRegion(idx1);
            S9sString cidr   = server.subnetCidr(idx1);
            S9sString vpcId  = server.subnetVpcId(idx1);
            S9sString id     = server.subnetId(idx1);

            cloudFormat.widen(cloud);
            regionFormat.widen(region);
            hostNameFormat.widen(hostName);
            cidrFormat.widen(cidr);
            vpcFormat.widen(vpcId);
            idFormat.widen(id);

            if (!regions.contains(region))
                regions << region;

            ++nSubnetsFound;
        }
    }
    
    if (!options->isNoHeaderRequested())
    {
        cloudFormat.widen("CLD");
        regionFormat.widen("REGION");
        hostNameFormat.widen("SERVER");
        cidrFormat.widen("CIDR");
        vpcFormat.widen("VPC");
        idFormat.widen("ID");

        printf("%s", headerColorBegin());
        cloudFormat.printf("CLD");
        regionFormat.printf("REGION");
        hostNameFormat.printf("SERVER");
        cidrFormat.printf("CIDR", false);
        vpcFormat.printf("VPC");
        idFormat.printf("ID");
        printf("%s", headerColorEnd());
        printf("\n");

    }
    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap   = theList[idx].toVariantMap();
        S9sServer      server   = theMap;
        S9sString      hostName = server.hostName();
        int            nSubnets = server.nSubnets();

        for (int idx1 = 0; idx1 < nSubnets; ++idx1)
        {
            S9sString cloud  = server.subnetProvider(idx1);
            S9sString region = server.subnetRegion(idx1);
            S9sString cidr   = server.subnetCidr(idx1);
            S9sString vpcId  = server.subnetVpcId(idx1);
            S9sString id     = server.subnetId(idx1);

            if (region.empty())
                region = "-";

            hostNameFormat.setColor(
                    server.colorBegin(syntaxHighlight),
                    server.colorEnd(syntaxHighlight));
            
            cloudFormat.printf(cloud);
            regionFormat.printf(region);
            hostNameFormat.printf(hostName);
            cidrFormat.printf(cidr);
            vpcFormat.printf(vpcId);
            idFormat.printf(id);

            ::printf("\n");
        }
    }
    
    if (!options->isBatchRequested())
    {
        ::printf("Total %s%d%s subnet(s) in %s%d%s region(s).\n",
                numberColorBegin(), nSubnetsFound, numberColorEnd(),
                numberColorBegin(), (int)regions.size(), numberColorEnd()
                );
    }
}

void
S9sRpcReply::printRegions()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    S9sFormat       hasCredentialsFormat;
    S9sFormat       providerFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       nameFormat;
    int             nLines = 0;
    int             nRegions = 0;
    S9sVariantMap   cloudMap;

    // If the json is requested we simply print it and that's all.
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }
    
    /*
     * Collecting information for the long list format.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sServer      server   = theList[idx].toVariantMap();
        S9sVariantList regions  = server.regions();
        S9sString      hostName = server.hostName();

        for (uint idx1 = 0u; idx1 < regions.size(); ++idx1)
        {
            S9sVariantMap regionMap = regions[idx1].toVariantMap();
            S9sString     name = regionMap["name"].toString();
            S9sString     provider = regionMap["provider"].toString();
            S9sString     hasCredentials;

            // Statistics.
            ++nRegions;
            cloudMap[provider] = true;

            // Filtering.
            if (!options->isStringMatchExtraArguments(name))
                continue;

            if (!options->cloudName().empty() &&
                    options->cloudName() != provider)
            {
                continue;
            }
        
            hasCredentials = 
                regionMap["has_credentials"].toBoolean() ? "Y" : "N";
        
            hasCredentialsFormat.widen(hasCredentials);
            providerFormat.widen(provider);
            hostNameFormat.widen(hostName);
            nameFormat.widen(name);

            ++nLines;
        }
    }
    
    if (!options->isNoHeaderRequested() && nLines > 0)
    {
        hasCredentialsFormat.widen("CRED");
        providerFormat.widen("CLOUD");
        hostNameFormat.widen("SERVER");
        nameFormat.widen("REGION");

        printf("%s", headerColorBegin());
        hasCredentialsFormat.printf("CRED");
        providerFormat.printf("CLOUD");
        hostNameFormat.printf("SERVER");
        nameFormat.printf("REGION");
        printf("%s\n", headerColorEnd());
    }
    
    /*
     * Printing the long list.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sServer      server   = theList[idx].toVariantMap();
        S9sVariantList regions  = server.regions();
        S9sString      hostName = server.hostName();

        for (uint idx1 = 0u; idx1 < regions.size(); ++idx1)
        {
            S9sVariantMap regionMap = regions[idx1].toVariantMap();
            S9sString     name = regionMap["name"].toString();
            S9sString     provider = regionMap["provider"].toString();
            S9sString     hasCredentials;

            // Filtering.
            if (!options->isStringMatchExtraArguments(name))
                continue;
            
            if (!options->cloudName().empty() &&
                    options->cloudName() != provider)
            {
                continue;
            }

            hasCredentials = 
                regionMap["has_credentials"].toBoolean() ? "Y" : "N";

            hostNameFormat.setColor(
                    server.colorBegin(syntaxHighlight),
                    server.colorEnd(syntaxHighlight));
           
            if (syntaxHighlight)
            {
                if (regionMap["has_credentials"].toBoolean())
                {
                    nameFormat.setColor(XTERM_COLOR_REGION_OK, TERM_NORMAL);
                } else {
                    nameFormat.setColor(XTERM_COLOR_REGION_FAIL, TERM_NORMAL);
                }
            }

            hasCredentialsFormat.printf(hasCredentials);
            providerFormat.printf(provider);
            hostNameFormat.printf(hostName);
            nameFormat.printf(name);

            ::printf("\n");
        }
    }
    
    if (!options->isBatchRequested())
    {
        printf("Total: %s%d%s regions in %s%zu%s clouds.\n", 
                numberColorBegin(), nRegions, numberColorEnd(),
                numberColorBegin(), cloudMap.size(), numberColorEnd());
    }
}

void
S9sRpcReply::printImages()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
    } else if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
    } else if (options->isLongRequested()) 
    {
        printImagesLong();
    } else {
        printImagesBrief();
    }
}

void
S9sRpcReply::printImagesBrief()
{
    //S9sOptions     *options = S9sOptions::instance();
    //bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    S9sStringList   collectedList;

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList images = theMap["images"].toVariantList();

        for (uint idx1 = 0u; idx1 < images.size(); ++idx1)
        {
            S9sVariantMap imageMap = images[idx1].toVariantMap();
            S9sString     image    = imageMap["name"].toString();
            S9sString     cloud    = imageMap["provider"].toString();
            S9sString     region   = imageMap["region"].toString();

            /*
             * Filtering.
             */
            if (!options->cloudName().empty() && options->cloudName() != cloud)
                continue;

            if (!options->region().empty() && options->region() != region)
                continue;

            if (image.empty())
                continue;

            if (collectedList.contains(image))
                continue;

            collectedList << image;
        }
    }
    
    for (uint idx = 0; idx < collectedList.size(); ++idx)
        ::printf("%s ", STR(collectedList[idx]));

    ::printf("\n");
}

void
S9sRpcReply::printImagesLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    S9sFormat       cloudFormat;
    S9sFormat       regionFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       imageFormat;
    int             nLines = 0;
    int             nImages = 0;

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap   = theList[idx].toVariantMap();
        S9sServer      server   = theMap;
        S9sString      hostName = server.hostName();
        S9sVariantList images   = theMap["images"].toVariantList();

        if (!options->isStringMatchExtraArguments(hostName))
            continue;
       
        #if 0
        // Printing the server name in color.
        ::printf("%s%s%s:", 
                server.colorBegin(syntaxHighlight),
                STR(hostName),
                server.colorEnd(syntaxHighlight));
        #endif

        // Collecting all the images the given server supports.
        for (uint idx1 = 0u; idx1 < images.size(); ++idx1)
        {
            S9sVariantMap imageMap = images[idx1].toVariantMap();
            S9sString     image    = imageMap["name"].toString();
            S9sString     cloud    = imageMap["provider"].toString();
            S9sString     region   = imageMap["region"].toString();
            
            ++nImages;

            /*
             * Filtering.
             */
            if (!options->cloudName().empty() && options->cloudName() != cloud)
                continue;

            if (!options->region().empty() && options->region() != region)
                continue;

            /*
             *
             */
            cloudFormat.widen(cloud);
            regionFormat.widen(region);
            hostNameFormat.widen(hostName);
            imageFormat.widen(image);
            ++nLines;
        }
    }

    if (!options->isNoHeaderRequested() && nLines > 0)
    {
        cloudFormat.widen("CLD");
        regionFormat.widen("REGION");
        hostNameFormat.widen("SERVER");
        imageFormat.widen("IMAGE");

        printf("%s", headerColorBegin());
        cloudFormat.printf("CLD");
        regionFormat.printf("REGION");
        hostNameFormat.printf("SERVER");
        imageFormat.printf("IMAGE");
        printf("%s", headerColorEnd());
        printf("\n");

    }

    /*
     * Printing the table lines.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap   = theList[idx].toVariantMap();
        S9sServer      server   = theMap;
        S9sString      hostName = server.hostName();
        S9sVariantList images   = theMap["images"].toVariantList();

        if (!options->isStringMatchExtraArguments(hostName))
            continue;
       
        // Collecting all the images the given server supports.
        for (uint idx1 = 0u; idx1 < images.size(); ++idx1)
        {
            S9sVariantMap imageMap = images[idx1].toVariantMap();
            S9sString     image    = imageMap["name"].toString();
            S9sString     cloud    = imageMap["provider"].toString();
            S9sString     region   = imageMap["region"].toString();

            /*
             * Filtering.
             */
            if (!options->cloudName().empty() && options->cloudName() != cloud)
                continue;

            if (!options->region().empty() && options->region() != region)
                continue;

            /*
             *
             */
            if (region.empty())
                region = "-";

            cloudFormat.printf(cloud);
            regionFormat.printf(region);
        
            ::printf("%s", server.colorBegin(syntaxHighlight));
            hostNameFormat.printf(hostName);
            ::printf("%s", server.colorEnd(syntaxHighlight));

            imageFormat.printf(image);
            ::printf("\n");
        }
    }
    
    if (!options->isBatchRequested())
    {
        ::printf("Total %s%d%s image(s).\n",
                numberColorBegin(), nImages, numberColorEnd());
    }
}

void
S9sRpcReply::printServers()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
    } else if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
    } else if (options->isStatRequested())
    {
        printServersStat();
    } else if (options->isLongRequested()) 
    {
        printServersLong();
    } else {
        printServersBrief();
    }
}

/**
 * \param server The server to print.
 *
 * Prints one server in stat format, a format we use when the --stat command
 * line option is provided.
 */
void
S9sRpcReply::printServerStat(
        S9sServer server)
{
    S9sOptions    *options = S9sOptions::instance();
    int            terminalWidth = options->terminalWidth();
    const char    *greyBegin = greyColorBegin();
    const char    *greyEnd   = greyColorEnd();
    S9sString      title;
    S9sVariantList processorNames = server.processorNames();
    S9sVariantList nicNames       = server.nicNames();
    S9sVariantList bankNames      = server.memoryBankNames();
    S9sVariantList diskNames      = server.diskNames();
    S9sVariantList containers;

    //
    // The title line that is in inverse. 
    //
    if (server.hostName() == server.ipAddress())
    {
        title = server.hostName();
    } else {
        title.sprintf("%s (%s)", 
                STR(server.hostName()), STR(server.ipAddress()));
    }

    printf("%s", TERM_INVERSE);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL);
    printf("\n");

    printObjectStat(server);

    //
    // "      IP: 192.168.1.4"
    //
    printf("%s      IP:%s ", greyBegin, greyEnd);
    printf("%s%-33s%s ", 
            ipColorBegin(server.ipAddress()),
            STR(server.ipAddress()),
            ipColorEnd());

    printf("%sProtocol:%s ", greyBegin, greyEnd);
    printf("%-25s ", STR(server.protocol()));

    printf("\n");
    
    //
    //
    //
    printf("%s  Status:%s ", greyBegin, greyEnd);
    printf("%s%-24s%s ", 
            hostStateColorBegin(server.hostStatus()),
            STR(server.hostStatus()),
            hostStateColorEnd());

    printf("\n");
    
    //
    // ""
    //
    printf("%s      OS:%s ", greyBegin, greyEnd);
    printf("%-24s", STR(server.osVersionString("-")));
    printf("\n");
    

    // 
    // "   Alias: ''                        Owner: pipas/users" 
    //
    printf("%s   Alias:%s ", greyBegin, greyEnd);
    printf("%-16s ", STR(server.alias("-")));
    //printf("\n");
    
    printf("\n");
    
    //
    // "   Model: SUN FIRE X4170 SERVER (4583256-1)"
    //
    printf("%s   Model:%s ", greyBegin, greyEnd);
    printf("%-16s ", STR(server.model("-")));
    printf("\n");

    //
    //
    //
    printf("%s Summary:%s ", greyBegin, greyEnd);
    printf("%2d VMs", server.nContainers());
    printf(", %.0fGB RAM", server.totalMemoryGBytes());
    printf(", %d CPUs", server.nCpus());
    printf(", %d cores", server.nCores());
    printf(", %d threads", server.nThreads());
    printf("\n");

    //
    //
    //
    printf("%s  Limits:%s ", greyBegin, greyEnd);
    printf("%s/%s VMs", 
            STR(server.nContainersMaxString()),
            STR(server.nRunningContainersMaxString()));
    printf("\n");

    //
    // "  CPU(s): 2 x Intel(R) Xeon(R) CPU L5520 @ 2.27GHz"
    //
    for (uint idx = 0u; idx < processorNames.size(); ++idx)
    {
        if (idx == 0u)
        {
            printf("%s  CPU(s):%s ", greyBegin, greyEnd);
        } else {
            printf("          ");
        }

        printf("%s\n", STR(processorNames[idx].toString()));
    }

    //
    // "  NIC(s): 4 x 82575EB Gigabit Network Connection"
    //
    for (uint idx = 0u; idx < nicNames.size(); ++idx)
    {
        if (idx == 0u)
        {
            printf("%s  NIC(s):%s ", greyBegin, greyEnd);
        } else {
            printf("          ");
        }

        printf("%s\n", STR(nicNames[idx].toString()));
    }
    
    //
    // "   Banks: 16 x DIMM 800 MHz (1.2 ns)"
    //
    for (uint idx = 0u; idx < bankNames.size(); ++idx)
    {
        if (idx == 0u)
        {
            printf("%s   Banks:%s ", greyBegin, greyEnd);
        } else {
            printf("          ");
        }

        printf("%s\n", STR(bankNames[idx].toString()));
    }

    //
    // "   Disks:  4 x FUJITSU MBE2147RC"
    //
    for (uint idx = 0u; idx < diskNames.size(); ++idx)
    {
        if (idx == 0u)
        {
            printf("%s   Disks:%s ", greyBegin, greyEnd);
        } else {
            printf("          ");
        }

        printf("%s\n", STR(diskNames[idx].toString()));
    }
        
    //
    //
    //
    printf("\n");
    containers = server.containers();
    printContainersCompact(containers);

    printf("\n");
}

/**
 * Prints the servers in stat format, a format we use when the --stat command
 * line option is provided.
 */
void
S9sRpcReply::printServersStat()
{
    S9sVariantList  theList = operator[]("servers").toVariantList();
    S9sOptions     *options = S9sOptions::instance();
    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap   = theList[idx].toVariantMap();
        S9sServer      server   = theMap;
        S9sString      hostName = server.hostName();

        if (!options->isStringMatchExtraArguments(hostName))
            continue;
        
        printServerStat(server);
    }
}

/**
 * Prints the servers in the long format.
 */
void 
S9sRpcReply::printServersLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    int             total   = operator[]("total").toInt();
    int             nLines = 0;
    S9sFormat       protocolFormat;
    S9sFormat       versionFormat;
    S9sFormat       nContainersFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       ownerFormat;
    S9sFormat       groupFormat;
    S9sFormat       ipFormat(ipColorBegin(), ipColorEnd());

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap   = theList[idx].toVariantMap();
        S9sServer      server   = theMap;
        S9sString      hostName = server.hostName();
        S9sString      prot     = server.protocol();
        S9sString      version  = server.version();
        S9sString      owner    = server.ownerName();
        S9sString      group    = server.groupOwnerName();
        int            nContainers = server.nContainers();
        S9sString      ip       = server.ipAddress();

        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        if (version.empty())
            version = "-";
        
        if (prot.empty())
            prot = "-";

        protocolFormat.widen(prot);
        versionFormat.widen(version);
        nContainersFormat.widen(nContainers);
        hostNameFormat.widen(hostName);
        ownerFormat.widen(owner);
        groupFormat.widen(group);
        ipFormat.widen(ip);
        ++nLines;
    }

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested() && nLines > 0)
    {
        protocolFormat.widen("CLD");
        versionFormat.widen("VERSION");
        nContainersFormat.widen("#C");
        ownerFormat.widen("OWNER");
        groupFormat.widen("GROUP");
        hostNameFormat.widen("SERVER_NAME");
        ipFormat.widen("IP");
        
        printf("%s", headerColorBegin());
        protocolFormat.printf("CLD");
        versionFormat.printf("VERSION");
        nContainersFormat.printf("#C");
        ownerFormat.printf("OWNER");
        groupFormat.printf("GROUP");
        hostNameFormat.printf("NAME");
        ipFormat.printf("IP", false);
        printf("COMMENT");
        printf("%s\n", headerColorEnd());
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap   = theList[idx].toVariantMap();
        S9sServer      server   = theMap;
        S9sString      hostName = server.hostName();
        S9sString      prot     = theMap["protocol"].toString();
        S9sString      version  = theMap["version"].toString();
        S9sString      status   = server.status();
        S9sString      owner    = theMap["owner_user_name"].toString();
        S9sString      group    = theMap["owner_group_name"].toString();
        S9sString      message  = server.message("-");
        int            nContainers = theMap["containers"].size();
        S9sString      ip       = server.ipAddress("-");

        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        if (version.empty())
            version = "-";
        
        if (prot.empty())
            prot = "-";

        hostNameFormat.setColor(
                server.colorBegin(syntaxHighlight),
                server.colorEnd(syntaxHighlight));
        
        protocolFormat.printf(prot);
        versionFormat.printf(version);
        nContainersFormat.printf(nContainers);
        
        printf("%s", userColorBegin());
        ownerFormat.printf(owner);
        printf("%s", userColorEnd());
        
        printf("%s", groupColorBegin(group));
        groupFormat.printf(group);
        printf("%s", groupColorEnd());

        hostNameFormat.printf(hostName);
        ipFormat.printf(ip);

        printf("%s", STR(message));

        printf("\n");
    }

    if (!options->isBatchRequested())
        printf("Total: %d server(s)\n", total);
}

/**
 * Prints the servers in the brief format.
 */
void 
S9sRpcReply::printServersBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("servers").toVariantList();
    const char     *hostColorBegin = "";
    const char     *hostColorEnd = "";

    if (syntaxHighlight)
    {
        hostColorBegin  = XTERM_COLOR_GREEN;
        hostColorEnd    = TERM_NORMAL;
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap   = theList[idx].toVariantMap();
        S9sServer      server   = theMap;
        S9sString      hostName = server.hostName();

        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        printf("%s%s%s\n", hostColorBegin, STR(hostName), hostColorEnd);
    }
}

/**
 * \param serverName The name of the server holding the container or an empty
 *   string.
 * \param containerName The name of the container to return.
 * \returns The container object if the container is in the reply or an invalid
 *   container if the container was not found.
 */
S9sContainer
S9sRpcReply::container(
        const S9sString &serverName,
        const S9sString &containerName)
{
    S9sVariantList  theList = operator[]("containers").toVariantList();
    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sString      alias  = theMap["alias"].toString();
        S9sString      parent = theMap["parent_server"].toString();

        if (!serverName.empty() && serverName != parent)
            continue;

        if (containerName != alias)
            continue;

        return S9sContainer(theMap);
    }

    return S9sContainer();
}

/**
 * Prints the containers.
 */
void
S9sRpcReply::printContainers()
{
    S9sOptions *options = S9sOptions::instance();

    if (!isOk())
        PRINT_ERROR("%s", STR(errorString()));
    else if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (options->isStatRequested())
        printContainerStat();
    else if (options->isLongRequested())
        printContainersLong();
    else
        printContainersBrief();
}

/**
 * Prints the containers in long format.
 *
 * Here is how a container looks like.
 * \code{.js}
 * {
 *     "alias": "cmon_node_0001",
 *     "class_name": "CmonContainer",
 *     "hostname": "192.168.1.212",
 *     "ip": "192.168.1.212",
 *     "ipv4_addresses": [ "192.168.1.212" ],
 *     "owner_group_id": 1,
 *     "owner_user_id": 3,
 *     "parent_server": "core1",
 *     "status": "RUNNING"
 * }, 
 * \endcode
 */
void 
S9sRpcReply::printContainersLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       subnetId  = options->subnetId();
    S9sString       vpcId     = options->vpcId();
    S9s::AddressType addressType = options->addressType();
    int             terminalWidth = options->terminalWidth();
    int             nColumns;
    bool            truncate = options->truncate();
    S9sString       cloudName = options->cloudName();
    S9sVariantList  theList = operator[]("containers").toVariantList();
    S9sString       formatString = options->containerFormat();
    int             total   = operator[]("total").toInt();
    int             nLines = 0;
    int             totalRunning = 0;
    S9sFormat       typeFormat;
    S9sFormat       templateFormat;
    S9sFormat       ipFormat(ipColorBegin(), ipColorEnd());
    S9sFormat       userFormat(userColorBegin(), userColorEnd());
    S9sFormat       groupFormat;
    S9sFormat       parentFormat;

    if (options->hasContainerFormat())
    {
        for (uint idx = 0; idx < theList.size(); ++idx)
        {
            S9sVariantMap  theMap      = theList[idx].toVariantMap();
            S9sContainer   container(theMap);

            total += 1;
   
            if (!options->isStringMatchExtraArguments(container.name()))
                continue;

            if (!cloudName.empty() && container.provider() != cloudName)
                continue;
        
            if (!subnetId.empty() && container.subnetId() != subnetId)
                continue;
        
            if (!vpcId.empty() && vpcId != container.subnetVpcId())
                continue;

            printf("%s", 
                    STR(container.toString(syntaxHighlight, formatString)));
        }
    
        if (!options->isBatchRequested())
            printf("Total: %d\n", total); 

        return;
    }

    /*
     * First run-through: collecting some information.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sContainer   container(theMap);
        S9sString      alias  = container.name();
        S9sString      ip     = container.ipAddress(addressType, "-");
        S9sString      parent = theMap["parent_server"].toString();
        S9sString      user   = theMap["owner_user_name"].toString();
        S9sString      group  = theMap["owner_group_name"].toString();
        S9sString      type   = container.provider("-");
        S9sString      templateName = container.templateName("-", true);

        if (!options->isStringMatchExtraArguments(container.name()))
            continue;

        if (!cloudName.empty() && container.provider() != cloudName)
            continue;
        
        if (!subnetId.empty() && container.subnetId() != subnetId)
            continue;
        
        if (!vpcId.empty() && vpcId != container.subnetVpcId())
            continue;

        if (ip.empty())
            ip = "-";

        userFormat.widen(user);
        groupFormat.widen(group);
        ipFormat.widen(ip);
        parentFormat.widen(parent);
        typeFormat.widen(type);
        templateFormat.widen(templateName);
        ++nLines;
    }

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested() && nLines > 0)
    {
        typeFormat.widen("CLD");
        templateFormat.widen("TEMPLATE");
        userFormat.widen("OWNER");
        groupFormat.widen("GROUP");
        ipFormat.widen("IP ADDRESS");
        parentFormat.widen("SERVER");

        printf("%s", headerColorBegin());
        printf("S ");
        typeFormat.printf("CLD", false);
        templateFormat.printf("TEMPLATE", false);
        userFormat.printf("OWNER", false);
        groupFormat.printf("GROUP", false);
        ipFormat.printf("IP ADDRESS", false);
        parentFormat.printf("SERVER", false);
        ::printf("NAME");

        printf("%s\n", headerColorEnd());
    }

    nColumns  = 0;
    nColumns += 2;
    nColumns += typeFormat.realWidth();
    nColumns += templateFormat.realWidth();
    nColumns += userFormat.realWidth();
    nColumns += groupFormat.realWidth();
    nColumns += ipFormat.realWidth();
    nColumns += parentFormat.realWidth();
    
    
    /*
     * Second run: doing the actual printing.
     */    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sContainer   container(theMap);
        S9sString      alias  = container.name();
        S9sString      ip     = container.ipAddress(addressType, "-");
        bool           isRunning = theMap["status"] == "RUNNING";
        S9sString      parent = theMap["parent_server"].toString();
        S9sString      user   = theMap["owner_user_name"].toString();
        S9sString      group  = theMap["owner_group_name"].toString();
        S9sString      type   = container.provider("-");
        S9sString      templateName = container.templateName("-", true);

        if (isRunning)
            totalRunning++;
        
        if (!options->isStringMatchExtraArguments(alias))
            continue;
        
        if (!subnetId.empty() && container.subnetId() != subnetId)
            continue;
        
        if (!vpcId.empty() && vpcId != container.subnetVpcId())
            continue;

        if (!cloudName.empty() && container.provider() != cloudName)
            continue;

        if (ip.empty())
            ip = "-";

        if (truncate && nColumns < terminalWidth)
        {
            int remaining  = terminalWidth - nColumns;

            if (remaining < (int) alias.length())
            {
                alias.resize(remaining - 1);
                alias += "…";
            }  
        }
       
        printf("%c ", container.stateAsChar());
        
        typeFormat.printf(type);
        templateFormat.printf(templateName);

        printf("%s", userColorBegin());
        userFormat.printf(user);
        printf("%s", userColorEnd());
        
        printf("%s", groupColorBegin(group));
        groupFormat.printf(group);
        printf("%s", groupColorEnd());

        ipFormat.printf(ip);

        printf("%s", serverColorBegin());
        parentFormat.printf(parent);
        printf("%s", serverColorEnd());

        ::printf("%s%s%s", 
                containerColorBegin(container.stateAsChar()), 
                STR(alias),
                containerColorEnd());

        printf("\n");
    }
    
    if (!options->isBatchRequested())
    {
        printf("Total: %s%d%s containers, %s%d%s running.\n", 
                numberColorBegin(), total, numberColorEnd(),
                numberColorBegin(), totalRunning, numberColorEnd());
    }
}

void 
S9sRpcReply::printContainersBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       subnetId  = options->subnetId();
    S9sString       vpcId     = options->vpcId();
    S9sVariantList  theList = operator[]("containers").toVariantList();
    S9sString       cloudName = options->cloudName();
    S9sString       formatString = options->containerFormat();
    
    if (options->hasContainerFormat())
    {
        for (uint idx = 0; idx < theList.size(); ++idx)
        {
            S9sVariantMap  theMap      = theList[idx].toVariantMap();
            S9sContainer   container(theMap);

            if (!options->isStringMatchExtraArguments(container.name()))
                continue;

            if (!cloudName.empty() && container.provider() != cloudName)
                continue;
        
            if (!subnetId.empty() && container.subnetId() != subnetId)
                continue;
        
            if (!vpcId.empty() && vpcId != container.subnetVpcId())
                continue;

            printf("%s", 
                    STR(container.toString(syntaxHighlight, formatString)));
        }

        return;
    }

    /*
     * Printing the list.
     */    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sContainer   container(theMap);
        S9sString      alias  = container.name();

        if (!options->isStringMatchExtraArguments(alias))
            continue;
        
        if (!subnetId.empty() && container.subnetId() != subnetId)
            continue;
        
        if (!vpcId.empty() && vpcId != container.subnetVpcId())
            continue;

        if (!cloudName.empty() && container.provider() != cloudName)
            continue;

        ::printf("%s%s%s ", 
                containerColorBegin(container.stateAsChar()), 
                STR(alias),
                containerColorEnd());

    }
    
    printf("\n");
}

void
S9sRpcReply::printContainersCompact(
        const S9sVariantList &containers)
{
    S9sOptions    *options = S9sOptions::instance();
    S9s::AddressType addressType = options->addressType();
    int            terminalWidth = options->terminalWidth();
    S9sFormat      aliasFormat(containerColorBegin(), containerColorEnd());
    S9sFormat      ipFormat(ipColorBegin(), ipColorEnd());
    S9sFormat      userFormat(userColorBegin(), userColorEnd());
    S9sFormat      groupFormat(groupColorBegin(), groupColorEnd());
    S9sString      indent;
    int            tableWidth;

    if (containers.empty())
        return;

    for (uint idx = 0u; idx < containers.size(); ++idx)
    {
        const S9sContainer container = containers[idx].toContainer();
        S9sString          user      = container.ownerName();
        S9sString          group     = container.groupOwnerName();
        S9sString          alias     = container.alias();
        S9sString          ipAddress = container.ipAddress(addressType, "-");

        userFormat.widen(user);
        groupFormat.widen(group);
        aliasFormat.widen(alias);
        ipFormat.widen(ipAddress);
    }
    
    tableWidth = 
        3 +
        aliasFormat.realWidth()  + ipFormat.realWidth() +
        userFormat.realWidth()   + groupFormat.realWidth();
    
    if (terminalWidth - tableWidth > 0)
        indent = S9sString(" ") * ((terminalWidth - tableWidth) / 2);

    printf("%s", headerColorBegin());
    printf("%s", STR(indent));
    printf("S ");
    userFormat.printf("OWNER", false);
    groupFormat.printf("GROUP", false);
    ipFormat.printf("IP ADDRESS", false);
    aliasFormat.printf("NAME", false);
    printf("%s", headerColorEnd());
    printf("\n");


    for (uint idx = 0u; idx < containers.size(); ++idx)
    {
        const S9sContainer container = containers[idx].toContainer();
        S9sString          user      = container.ownerName();
        S9sString          group     = container.groupOwnerName();
        S9sString          alias     = container.alias();
        S9sString          ipAddress = container.ipAddress(addressType, "-");
        
        printf("%s", STR(indent));
        printf("%c ", container.stateAsChar());

        userFormat.printf(user);
        groupFormat.printf(group);
        ipFormat.printf(ipAddress);
        aliasFormat.printf(alias);
        printf("\n");
    }
}

/**
 * Prints the CDT in either a list or a tree format.
 */
void
S9sRpcReply::printObjectTree()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
    } else if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
    } else if (options->isLongRequested())
    {
        printObjectTreeBrief();
    } else {
        // This is the same with or without --long.
        printObjectTreeBrief();
    }
}

S9sString 
aclStringToUiString(
        S9sString aclString)
{
    S9sVariantList parts = aclString.split(",");
    S9sString user  = "---";
    S9sString group = "---";
    S9sString other = "---";
    S9sString extra = " ";

    for (uint idx = 0u; idx < parts.size(); ++idx)
    {
        S9sString part = parts[idx].toString();
        S9sString lastField = part.substr(part.find_last_of(":") + 1);

        if (part.startsWith("user::"))
            user = lastField;
        else if (part.startsWith("group::"))
            group = lastField;
        else if (part.startsWith("other::"))
            other = lastField;
        else 
            extra = "+";
    }

    return user + group + other + extra;
}

/**
 * \param recursionLevel Shows how deep we are in the printing (not in the tree,
 *   this might be a subtree).
 */
void 
S9sRpcReply::printObjectTreeBrief(
        S9sVariantMap        entry,
        int                  recursionLevel,
        S9sString            indentString,
        bool                 isLast)
{
    S9sOptions     *options   = S9sOptions::instance();
    bool            onlyAscii = options->onlyAscii();
    S9sString       name      = entry["item_name"].toString();
    S9sString       path      = entry["item_path"].toString();
    S9sString       spec      = entry["item_spec"].toString();
    S9sString       type      = entry["item_type"].toString();
    S9sVariantList  entries   = entry["sub_items"].toVariantList();
    S9sString       linkTarget = entry["link_target"].toString();
    bool            isDir     = type == "Folder";
    bool            isFile    = type == "File";
    bool            isCluster = type == "Cluster";
    bool            isNode    = type == "Node";
    bool            isServer  = type == "Server";
    bool            isUser    = type == "User";
    bool            isGroup   = type == "Group";
    bool            isContainer = type == "Container";
    bool            isDatabase = type == "Database";
    S9sString       indent;

    // It looks better if we print the full path on the first item when the
    // first item is not the root folder. The user will know where the tree
    // starts.
    if (recursionLevel == 0 && name != "/")
    {
        S9sString tmp;

        tmp  = path;
        if (!tmp.endsWith("/"))
            tmp += "/";

        tmp += name;

        name = tmp;
    }

    if (options->fullPathRequested())
    {
        name = S9sString::buildPath(path, name);
    }

    printf("%s", STR(indentString));

    if (recursionLevel)
    {
        if (isLast)
            indent = onlyAscii ? "+-- " : "└── ";
        else 
            indent = onlyAscii ? "+-- " : "├── ";
    }

    if (isDir)
    {
        printf("%s%s%s%s", 
                STR(indent), 
                folderColorBegin(), STR(name), folderColorEnd());
    } else if (isFile)
    {
        printf("%s%s%s%s", 
                STR(indent), 
                fileColorBegin(name), 
                STR(name), folderColorEnd());
    } else if (isContainer)
    {
        printf("%s%s%s%s", 
                STR(indent), 
                containerColorBegin(), STR(name), containerColorEnd());
    } else if (isCluster)
    {
        printf("%s%s%s%s", 
                STR(indent), 
                clusterColorBegin(), STR(name), clusterColorEnd());
    } else if (isNode)
    {
        printf("%s%s%s%s", 
                STR(indent), 
                ipColorBegin(), STR(name), ipColorEnd());
    } else if (isServer)
    {
        printf("%s%s%s%s", 
                STR(indent), 
                serverColorBegin(), STR(name), serverColorEnd());
    } else if (isUser)
    {
        printf("%s%s%s%s", 
                STR(indent), 
                userColorBegin(), STR(name), userColorEnd());
    } else if (isGroup)
    {
        printf("%s%s%s%s", 
                STR(indent), 
                groupColorBegin(), STR(name), groupColorEnd());
    } else if (isDatabase)
    {
        printf("%s%s%s%s", 
                STR(indent), 
                databaseColorBegin(), STR(name), databaseColorEnd());
    } else {
        printf("%s%s", STR(indent), STR(name));
    }

    if (!linkTarget.empty())
        printf(" -> %s", STR(linkTarget));
    else if (!spec.empty())
        printf(" (%s)", STR(spec));

    printf("\n");

    for (uint idx = 0; idx < entries.size(); ++idx)
    {
        S9sVariantMap child = entries[idx].toVariantMap();
        bool          last = idx + 1 >= entries.size();
     
        // Hidden entries are printed only if the --all command line option is
        // provided.
        if (child["item_name"].toString().startsWith(".") &&
                !options->isAllRequested())
        {
            continue;
        }

        if (recursionLevel)
        {
            if (isLast)
                indent = "    ";
            else
                indent = onlyAscii ? "|   " : "│   ";
        }

        printObjectTreeBrief(
                child, recursionLevel + 1, 
                indentString + indent, last);
    }
}

void
S9sRpcReply::walkObjectTree(
        S9sVariantMap       &node)
{
    S9sOptions     *options   = S9sOptions::instance();
    S9sString       name      = node["item_name"].toString();
    S9sString       owner     = node["owner_user_name"].toString();
    S9sString       group     = node["owner_group_name"].toString();
    S9sVariantList  entries   = node["sub_items"].toVariantList();
    S9sString       type      = node["item_type"].toString();
    S9sString       sizeString;

    if (node.contains("major_device_number") && 
            node.contains("minor_devide_number"))
    {
        int major = node["major_device_number"].toInt();
        int minor = node["minor_devide_number"].toInt();

        sizeString.sprintf("%d, %d", major, minor);
    } else if (node.contains("size"))
    {
        ulonglong size = node["size"].toULongLong();
        sizeString.sprintf("%'llu", size);
    } else {
        sizeString = "-";
    }

    m_ownerFormat.widen(owner);
    m_groupFormat.widen(group);
    m_sizeFormat.widen(sizeString);

    if (type == "Folder")
        m_numberOfFolders++;
    else
        m_numberOfObjects++;

    for (uint idx = 0; idx < entries.size(); ++idx)
    {
        S9sVariantMap child = entries[idx].toVariantMap();
        
        if (child["item_name"].toString().startsWith(".") && 
                !options->isAllRequested())
        {
            continue;
        }

        walkObjectTree(child);
    }
}

/**
 * \param recursionLevel Shows how deep we are in the printing (not in the tree,
 *   this might be a subtree).
 *
 * \code{.js}
 * $ s9s tree --list --long
 * MODE        SIZE OWNER  GROUP     NAME
 * drwxrwxr--  1, 0 system admins    groups
 * drwxrwxrwx     - pipas  testgroup home
 * urwxr--r--     - nobody admins    nobody
 * urwxr--r--     - pipas  admins    pipas
 * urwxr--r--     - system admins    system
 * Total: 7 object(s) in 4 folder(s).
 * \endcode
 */
void 
S9sRpcReply::printObjectListLong(
        S9sVariantMap        entry,
        int                  recursionLevel,
        S9sString            indentString,
        bool                 isLast)
{
    S9sTreeNode     node      = entry;
    S9sOptions     *options   = S9sOptions::instance();
    bool            recursive = options->isRecursiveRequested();
    bool            directory = options->isDirectoryRequested();
    S9sString       path      = entry["item_path"].toString();
    S9sString       spec      = entry["item_spec"].toString();
    S9sString       type      = entry["item_type"].toString();
    S9sVariantList  entries   = entry["sub_items"].toVariantList();
    S9sString       owner     = entry["owner_user_name"].toString();
    S9sString       group     = entry["owner_group_name"].toString();
    S9sString       acl       = entry["item_acl"].toString();
    S9sString       linkTarget = entry["link_target"].toString();
    S9sString       fullPath;
    S9sString       name;
    S9sString       sizeString;

    // If the first level is the directory, we skip it if we are not requested
    // to print the directory itself.
    if (recursionLevel == 0 && node.isFolder() && !directory)
        goto recursive_print;
    
    if (!recursive && !directory && recursionLevel > 1)
        return;
    
    if (!recursive && directory && recursionLevel > 0)
        return;

    //printf("%3d ", recursionLevel);

    if (owner.empty())
        owner.sprintf("%d", entry["owner_user_id"].toInt());
    
    if (group.empty())
        group.sprintf("%d", entry["owner_group_id"].toInt());

    fullPath = path;
    if (!fullPath.endsWith("/"))
        fullPath += "/";

    fullPath += node.name();

    if (options->fullPathRequested())
    {
        name = fullPath;
        S9S_WARNING("Full path: %s", STR(name));
    } else {
        name = node.name();
    }

    /*
     * The type and then the acl string.
     */
    if (type == "Folder")
        printf("d");
    if (type == "File")
        printf("-");
    else if (type == "Cluster")
        printf("c");
    else if (type == "Node")
        printf("n");
    else if (type == "Server")
        printf("s");
    else if (type == "User")
        printf("u");
    else if (type == "Group")
        printf("g");
    else if (type == "Container")
        printf("c");
    else if (type == "Database")
        printf("b");
   
    ::printf("%s", STR(aclStringToUiString(acl)));
    ::printf(" ");

    if (entry.contains("major_device_number") && 
            entry.contains("minor_devide_number"))
    {
        int major = entry["major_device_number"].toInt();
        int minor = entry["minor_devide_number"].toInt();

        sizeString.sprintf("%d, %d", major, minor);
    } else if (entry.contains("size")) 
    {
        ulonglong size = entry["size"].toULongLong();
        sizeString.sprintf("%'llu", size);
    } else {
        sizeString = "-";
    }

    m_sizeFormat.printf(sizeString);

    /*
     * The owner and the group owner.
     */
    ::printf("%s", userColorBegin());
    m_ownerFormat.printf(owner);
    ::printf("%s", userColorEnd());

    ::printf("%s", groupColorBegin(group));
    m_groupFormat.printf(group);
    ::printf("%s", groupColorEnd());

    /*
     * The name.
     */
    if (type == "Folder")
    {
        printf("%s%s%s", 
                folderColorBegin(), 
                STR(name), 
                folderColorEnd());
    } else if (type == "File")
    {
        printf("%s%s%s", 
                fileColorBegin(name), 
                STR(name), 
                folderColorEnd());
    } else if (type == "Cluster")
    {
        printf("%s%s%s", 
                clusterColorBegin(), 
                STR(name), 
                clusterColorEnd());
    } else if (type == "Node")
    {
        printf("%s%s%s", 
                ipColorBegin(), 
                STR(name), 
                ipColorEnd());
    } else if (type == "Server")
    {
        printf("%s%s%s", 
                serverColorBegin(), 
                STR(name), 
                serverColorEnd());
    } else if (type == "User")
    {
        printf("%s%s%s", 
                userColorBegin(), 
                STR(name), 
                userColorEnd());
    } else if (type == "Group")
    {
        printf("%s%s%s", 
                groupColorBegin(), 
                STR(name), 
                groupColorEnd());
    } else if (type == "Container")
    {
        printf("%s%s%s", 
                containerColorBegin(), 
                STR(name), 
                containerColorEnd());
    } else if (type == "Database")
    {
        printf("%s%s%s", 
                databaseColorBegin(),
                STR(name), 
                databaseColorEnd());
    } else {
        printf("%s", STR(name));
    }

    if (!linkTarget.empty())
        printf(" -> %s", STR(linkTarget));

    printf("\n");

recursive_print:

    {
        for (uint idx = 0; idx < entries.size(); ++idx)
        {
            S9sVariantMap child = entries[idx].toVariantMap();
            bool          last = idx + 1 >= entries.size();
       
            if (child["item_name"].toString().startsWith(".") && 
                    !options->isAllRequested())
            {
                continue;
            }

            printObjectListLong(
                    child, recursionLevel + 1, 
                    "", last);
        }
    }
}

void 
S9sRpcReply::printObjectListBrief(
        S9sVariantMap        entry,
        int                  recursionLevel,
        S9sString            indentString,
        bool                 isLast)
{
    S9sTreeNode     node      = entry;
    S9sOptions     *options   = S9sOptions::instance();
    bool            recursive = options->isRecursiveRequested();
    bool            directory = options->isDirectoryRequested();
    S9sString       path      = entry["item_path"].toString();
    S9sString       spec      = entry["item_spec"].toString();
    S9sString       type      = entry["item_type"].toString();
    S9sVariantList  entries   = entry["sub_items"].toVariantList();
    S9sString       owner     = entry["owner_user_name"].toString();
    S9sString       group     = entry["owner_group_name"].toString();
    S9sString       acl       = entry["item_acl"].toString();
    S9sString       linkTarget = entry["link_target"].toString();
    S9sString       fullPath;
    S9sString       name;
    S9sString       sizeString;

    // If the first level is the directory, we skip it if we are not requested
    // to print the directory itself.
    if (recursionLevel == 0 && node.isFolder() && !directory)
        goto recursive_print;
    
    if (!recursive && !directory && recursionLevel > 1)
        return;
    
    if (!recursive && directory && recursionLevel > 0)
        return;

    //printf("%3d ", recursionLevel);

    if (owner.empty())
        owner.sprintf("%d", entry["owner_user_id"].toInt());
    
    if (group.empty())
        group.sprintf("%d", entry["owner_group_id"].toInt());

    fullPath = path;
    if (!fullPath.endsWith("/"))
        fullPath += "/";

    fullPath += node.name();

    if (options->fullPathRequested())
        name = fullPath;
    else
        name = node.name();
 
    /*
     * The name.
     */
    if (type == "Folder")
    {
        printf("%s%s%s", 
                folderColorBegin(), 
                STR(name), 
                folderColorEnd());
    } else if (type == "File")
    {
        printf("%s%s%s", 
                fileColorBegin(name), 
                STR(node.name()), 
                folderColorEnd());
    } else if (type == "Cluster")
    {
        printf("%s%s%s", 
                clusterColorBegin(), 
                STR(name), 
                clusterColorEnd());
    } else if (type == "Node")
    {
        printf("%s%s%s", 
                ipColorBegin(), 
                STR(name), 
                ipColorEnd());
    } else if (type == "Server")
    {
        printf("%s%s%s", 
                serverColorBegin(), 
                STR(name), 
                serverColorEnd());
    } else if (type == "User")
    {
        printf("%s%s%s", 
                userColorBegin(), 
                STR(name), 
                userColorEnd());
    } else if (type == "Group")
    {
        printf("%s%s%s", 
                groupColorBegin(), 
                STR(name), 
                groupColorEnd());
    } else if (type == "Container")
    {
        printf("%s%s%s", 
                containerColorBegin(), 
                STR(name), 
                containerColorEnd());
    } else if (type == "Database")
    {
        printf("%s%s%s", 
                databaseColorBegin(),
                STR(name), 
                databaseColorEnd());
    } else {
        printf("%s", STR(name));
    }

    if (!linkTarget.empty())
        printf(" -> %s", STR(linkTarget));

    printf("\n");

recursive_print:
    {
        for (uint idx = 0; idx < entries.size(); ++idx)
        {
            S9sVariantMap child = entries[idx].toVariantMap();
            bool          last = idx + 1 >= entries.size();
       
            if (child["item_name"].toString().startsWith(".") && 
                    !options->isAllRequested())
            {
                continue;
            }

            printObjectListBrief(
                    child, recursionLevel + 1, 
                    "", last);
        }
    }
}

/**
 * Prints the CDT in its tree format.
 */
void
S9sRpcReply::printObjectTreeBrief()
{
    S9sVariantMap entry =  operator[]("cdt").toVariantMap();
    printObjectTreeBrief(entry, 0, "", false);
}

/**
 * Prints the CDT as a list.
 */
void
S9sRpcReply::printObjectList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
    } else if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
    } else if (options->isLongRequested())
    {
        printObjectListLong();
    } else {
        printObjectListBrief(); 
    }
}

/**
 * Prints the CDT as a list.
 *
 * \code{.js}
 * MODE        SIZE OWNER  GROUP     NAME
 * drwxrwxr--  1, 0 system admins    groups
 * drwxrwxrwx     - pipas  testgroup home
 * urwxr--r--     - nobody admins    nobody
 * urwxr--r--     - pipas  admins    pipas
 * urwxr--r--     - system admins    system
 * Total: 7 object(s) in 4 folder(s).
 * \endcode
 */
void
S9sRpcReply::printObjectListLong()
{
    S9sOptions     *options = S9sOptions::instance();

    S9sVariantMap   entry   =  operator[]("cdt").toVariantMap();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    m_sizeFormat = S9sFormat();
    m_sizeFormat.setRightJustify();

    m_ownerFormat = S9sFormat();
    m_groupFormat = S9sFormat();
    m_numberOfObjects = 0;
    m_numberOfFolders = 0;

    walkObjectTree(entry);

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        m_sizeFormat.widen("SIZE");
        m_ownerFormat.widen("OWNER");
        m_groupFormat.widen("GROUP");

        printf("%s", headerColorBegin());
        printf("MODE        ");
        m_sizeFormat.printf("SIZE");
        m_ownerFormat.printf("OWNER");
        m_groupFormat.printf("GROUP");
        printf("NAME");
        printf("%s\n", headerColorEnd());

    }

    printObjectListLong(entry, 0, "", false);
        
    if (!options->isBatchRequested())
    {
        printf("Total: %d object(s) in %d folder(s).\n", 
                m_numberOfObjects,
                m_numberOfFolders);
    }
}

void
S9sRpcReply::printObjectListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   entry   =  operator[]("cdt").toVariantMap();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    m_numberOfObjects = 0;
    m_numberOfFolders = 0;

    walkObjectTree(entry);

    printObjectListBrief(entry, 0, "", false);
}

void
S9sRpcReply::printScriptOutput()
{
    S9sOptions *options = S9sOptions::instance();
    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (!isOk())
        PRINT_ERROR("%s", STR(errorString()));
    else 
        printScriptOutputBrief();
}

void
S9sRpcReply::printScriptBacktrace()
{
    S9sVariantMap   results  = operator[]("results").toVariantMap();
    S9sVariantList  backtrace = results["backtrace"].toVariantList();

    if (!backtrace.empty())
    {
        printf("\nBacktrace:\n");
    }

    for (uint idx = 0; idx < backtrace.size(); ++idx)
    {
        S9sString message = backtrace[idx].toString();

        printf("  %s\n", STR(message));
    }
}

void
S9sRpcReply::printScriptOutputBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantMap   results  = operator[]("results").toVariantMap();
    S9sVariantList  messages = results["messages"].toVariantList();

    for (uint idx = 0; idx < messages.size(); ++idx)
    {
        S9sVariantMap  theMap     = messages[idx].toVariantMap();
        S9sMessage     message    = theMap;

        if (syntaxHighlight)
        {
            printf("%s\n", STR(message.termColorString()));
        } else {
            printf("%s\n", STR(message.toString()));
        }
    }

    printScriptBacktrace();
}

void
S9sRpcReply::printReport()
{
    S9sVariantMap   reportMap = operator[]("report").toVariantMap();
    S9sReport       report(reportMap);

    printf("%s", STR(report.content()));
}

/**
 * \param a the first host represented as a variantmap
 * \param b the second host represented as a variantmap
 *
 * This function is used to sort the hosts before printing them in a list. First
 * the cluster IDs are compared, the second key is the host name.
 */
bool 
compareHostMaps(
        const S9sVariant &a,
        const S9sVariant &b)
{
    S9sVariantMap aMap       = a.toVariantMap();
    S9sVariantMap bMap       = b.toVariantMap();
    int           clusterId1 = aMap["clusterid"].toInt();
    int           clusterId2 = bMap["clusterid"].toInt();
    S9sString     hostName1  = aMap["hostname"].toString();
    S9sString     hostName2  = bMap["hostname"].toString();

    if (clusterId1 != clusterId2)
        return clusterId1 < clusterId2;

    return hostName1 < hostName2;
}

void 
S9sRpcReply::printHostTable(
        S9sCluster &cluster)
{
    S9sOptions    *options = S9sOptions::instance();
    int            terminalWidth = options->terminalWidth();
    S9sVariantList hostIds = cluster.hostIds();
    S9sFormat      hostNameFormat;
    S9sFormat      coresFormat;
    S9sFormat      memTotalFormat;
    S9sFormat      memUsedFormat;
    S9sFormat      cpuUsageFormat;
    S9sFormat      totalDiskFormat;
    S9sFormat      freeDiskFormat;
    S9sFormat      labelFormat;
    S9sFormat      swapTotalFormat;
    S9sFormat      swapFreeFormat;
    S9sFormat      rxSpeedFormat;
    S9sFormat      txSpeedFormat;
    int            tableWidth;
    S9sString      indent;

    memUsedFormat.setRightJustify();
    cpuUsageFormat.setRightJustify();
    rxSpeedFormat.setRightJustify();
    txSpeedFormat.setRightJustify();

    for (uint idx = 0u; idx < hostIds.size(); ++idx)
    {
        int        hostId    = hostIds[idx].toInt();
        S9sString  hostName  = cluster.hostName(hostId);
        S9sVariant nCores    = cluster.nCpuCores(hostId);
        S9sVariant memTotal  = cluster.memTotal(hostId);
        S9sVariant memUsed   = cluster.memUsed(hostId);
        S9sVariant cpuUsage  = cluster.cpuUsagePercent(hostId);
        S9sVariant totalDisk = cluster.totalDiskBytes(hostId);
        S9sVariant freeDisk  = cluster.freeDiskBytes(hostId);
        S9sVariant swapTotal = cluster.swapTotal(hostId);
        S9sVariant swapFree  = cluster.swapFree(hostId);
        S9sVariant rxSpeed   = cluster.rxBytesPerSecond(hostId);
        S9sVariant txSpeed   = cluster.txBytesPerSecond(hostId);
        

        hostNameFormat.widen(hostName);

        coresFormat.widen(nCores.toInt());
        cpuUsageFormat.widen(cpuUsage.toString(S9sVariant::IntegerNumber)+"%");

        memTotalFormat.widen(memTotal.toString(S9sVariant::BytesShort));
        memUsedFormat.widen(memUsed.toString(S9sVariant::BytesShort));

        totalDiskFormat.widen(totalDisk.toString(S9sVariant::BytesShort));
        freeDiskFormat.widen(freeDisk.toString(S9sVariant::BytesShort));

        swapTotalFormat.widen(swapTotal.toString(S9sVariant::BytesShort));
        swapFreeFormat.widen(swapFree.toString(S9sVariant::BytesShort));

        rxSpeedFormat.widen(rxSpeed.toString(S9sVariant::BytesPerSecShort));
        txSpeedFormat.widen(txSpeed.toString(S9sVariant::BytesPerSecShort));
    }
    
    tableWidth = 
        hostNameFormat.realWidth() + coresFormat.realWidth() +
        memTotalFormat.realWidth() + memUsedFormat.realWidth() +
        cpuUsageFormat.realWidth() + totalDiskFormat.realWidth() +
        freeDiskFormat.realWidth() + labelFormat.realWidth() +
        rxSpeedFormat.realWidth()  + txSpeedFormat.realWidth();

    if (terminalWidth - tableWidth > 0)
        indent = S9sString(" ") * ((terminalWidth - tableWidth) / 2);

    hostNameFormat.widen("HOSTNAME");

    printf("%s", headerColorBegin());
    printf("%s", STR(indent));

    hostNameFormat.printf("HOSTNAME");
    
    labelFormat = coresFormat + cpuUsageFormat;
    labelFormat.setCenterJustify();
    labelFormat.printf("CPU");

    labelFormat = memTotalFormat + memUsedFormat;
    labelFormat.setCenterJustify();
    labelFormat.printf("MEMORY");
    
    labelFormat = swapTotalFormat + swapFreeFormat;
    labelFormat.setCenterJustify();
    labelFormat.printf("SWAP"); 

    labelFormat = totalDiskFormat + freeDiskFormat;
    labelFormat.setCenterJustify();
    labelFormat.printf("DISK"); 
    
    labelFormat = rxSpeedFormat + txSpeedFormat;
    labelFormat.setCenterJustify();
    labelFormat.printf("NICs"); 

    printf("%s", headerColorEnd());
    printf("\n");

        
    for (uint idx = 0u; idx < hostIds.size(); ++idx)
    {
        int        hostId    = hostIds[idx].toInt();
        S9sString  hostName  = cluster.hostName(hostId);
        S9sVariant nCores    = cluster.nCpuCores(hostId);
        S9sVariant memTotal  = cluster.memTotal(hostId);
        S9sVariant memUsed   = cluster.memUsed(hostId);
        S9sVariant cpuUsage  = cluster.cpuUsagePercent(hostId);
        S9sVariant totalDisk = cluster.totalDiskBytes(hostId);
        S9sVariant freeDisk  = cluster.freeDiskBytes(hostId);
        S9sVariant swapTotal = cluster.swapTotal(hostId);
        S9sVariant swapFree  = cluster.swapFree(hostId);
        S9sVariant rxSpeed   = cluster.rxBytesPerSecond(hostId);
        S9sVariant txSpeed   = cluster.txBytesPerSecond(hostId);

        printf("%s", STR(indent));
        hostNameFormat.printf(hostName);
        coresFormat.printf(nCores.toInt());
        cpuUsageFormat.printf(
                cpuUsage.toString(S9sVariant::IntegerNumber) + "%");

        memTotalFormat.printf(memTotal.toString(S9sVariant::BytesShort));
        memUsedFormat.printf(memUsed.toString(S9sVariant::BytesShort));
        
        swapTotalFormat.printf(swapTotal.toString(S9sVariant::BytesShort));
        swapFreeFormat.printf(swapFree.toString(S9sVariant::BytesShort));
        
        totalDiskFormat.printf(totalDisk.toString(S9sVariant::BytesShort));
        freeDiskFormat.printf(freeDisk.toString(S9sVariant::BytesShort));
        
        rxSpeedFormat.printf(rxSpeed.toString(S9sVariant::BytesPerSecShort));
        txSpeedFormat.printf(txSpeed.toString(S9sVariant::BytesPerSecShort));
        printf("\n");
    }

    printf("\n");
}

/**
 * Prints one cluster in "stat" format, a very detailed format that is
 * used when the --stat command line option is provided.
 */
void
S9sRpcReply::printClusterStat(
        S9sCluster &cluster)
{
    S9sOptions *options = S9sOptions::instance();
    int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    S9sString   title;

    //
    // The title that is in inverse. 
    //
    title.sprintf(" %s ", STR(cluster.name()));

    ::printf("%s", TERM_INVERSE);
    ::printf("%s", STR(title));
    
    for (int n = title.length(); n < terminalWidth; ++n)
        ::printf(" ");

    ::printf("\n");
    ::printf("%s", TERM_NORMAL);
    
    //
    // "    Name: galera_001                          Owner: pipas/users"
    //
    printf("%s    Name:%s ", greyBegin, greyEnd);
    printf("%s", clusterColorBegin());
    printf("%-32s ", STR(cluster.name()));
    printf("%s", clusterColorEnd());
    
    printf("%s   Owner:%s ", greyBegin, greyEnd);
    printf("%s%s%s/%s%s%s ", 
            userColorBegin(), STR(cluster.ownerName()), userColorEnd(),
            groupColorBegin(cluster.groupOwnerName()), 
            STR(cluster.groupOwnerName()), 
            groupColorEnd());

    printf("\n");

    //
    // "      ID: 1                                   State: STARTED"
    //
    printf("%s      ID:%s ", greyBegin, greyEnd);
    printf("%-32d ", cluster.clusterId());
    
    printf("%s   State:%s ", greyBegin, greyEnd);
    printf("%s%s%s ", 
            clusterStateColorBegin(cluster.state()), 
            STR(cluster.state()),
            clusterStateColorEnd());
    printf("\n");
    
    printf("%s    Type:%s ", greyBegin, greyEnd);
    printf("%-32s ", STR(cluster.clusterType()));
    
    printf("%s  Vendor:%s ", greyBegin, greyEnd);
    printf("%s", STR(cluster.vendorAndVersion()));
    printf("\n");
    
    printf("%s  Status:%s ", greyBegin, greyEnd);
    printf("%s", STR(cluster.statusText()));
    printf("\n");

    //
    // Counting the alarms.
    //
    printf("%s  Alarms:%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.alarmsCritical());
    printf("%scrit %s ", greyBegin, greyEnd);
    printf("%2d ", cluster.alarmsWarning());
    printf("%swarn %s ", greyBegin, greyEnd);
    printf("\n");

    //
    // Counting the jobs.
    //
    printf("%s    Jobs:%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsAborted());
    printf("%sabort%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsDefined());
    printf("%sdefnd%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsDequeued());
    printf("%sdequd%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsFailed());
    printf("%sfaild%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsFinished());
    printf("%sfinsd%s ", greyBegin, greyEnd);
    printf("%2d ", cluster.jobsRunning());
    printf("%srunng%s ", greyBegin, greyEnd);
    printf("\n");
    
    //
    // Lines of various files.
    //
    printf("%s  Config:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(cluster.configFile()),
            STR(cluster.configFile()),
            fileColorEnd());

    printf("\n");

    printf("%s LogFile:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(cluster.logFile()),
            STR(cluster.logFile()),
            fileColorEnd());
    printf("\n");
    printf("\n");

    printHostTable(cluster);
}
        
bool 
S9sRpcReply::createGraph(
        S9sVector<S9sCmonGraph *> &graphs, 
        S9sNode                   &host,
        const S9sString           &filterName,
        const S9sVariant          &filterValue)
{
    S9sOptions           *options = S9sOptions::instance();
    S9sString             graphType = options->graph().toLower();
    bool                  syntaxHighlight = options->useSyntaxHighlight();
    const S9sVariantList &data = operator[]("data").toVariantList();
    S9sCmonGraph         *graph = NULL;
    bool                  success;

    /*
     *
     */
    S9S_DEBUG("Creating graph for %s.", STR(host.hostName()));
    graph = new S9sCmonGraph;
    graph->setNode(host);
    graph->setColor(syntaxHighlight);
    graph->setFilter(filterName, filterValue);
    graph->setShowDensity(options->density());

    success = graph->setGraphType(graphType);
    if (!success)
    {
        delete graph;
        PRINT_ERROR("The graph type '%s' is unrecognized.", STR(graphType));
        return false;
    }

    /*
     * Pushing the data into the graph.
     */
    for (uint idx = 0u; idx < data.size(); ++idx)
        graph->appendValue(data[idx].toVariantMap());


    graph->realize();
    graphs << graph;

    return true;
}

/**
 * Under construction.
 */
bool
S9sRpcReply::createGraph(
        S9sVector<S9sCmonGraph *> &graphs,
        S9sNode                   &host)
{
    const S9sVariantList &data = operator[]("data").toVariantList();
    S9sVariant            firstSample = data.empty() ? S9sVariant() : data[0];
    S9sString             filterName;
    S9sVariantList        filterValues;
    bool                  success = true;

    if (firstSample.contains("mountpoint"))
    {
        filterName = "mountpoint";
    } else if (firstSample.contains("interface"))
    {
        filterName = "interface";
    }

    if (!filterName.empty())
    {
        for (uint idx = 0u; idx < data.size(); ++idx)
        {
            S9sVariant map   = data[idx].toVariantMap();
            S9sVariant value = map[filterName];

            if (map["hostid"].toInt() != host.id())
                continue;

            if (!filterValues.contains(value))
            {
                S9S_DEBUG("-> '%s'", STR(value.toString()));
                filterValues << value;
            }
        }
    }

    S9S_DEBUG("filterValues.size() = %u", filterValues.size());
    if (filterValues.empty())
    {
        success = createGraph(graphs, host, filterName, S9sVariant());
    } else {
        for (uint idx = 0; idx < filterValues.size(); ++idx)
        {
            success = createGraph(graphs, host, filterName, filterValues[idx]);
            if (!success)
                break;
        }
    }

    return true;
}

/**
 * Under construction.
 */
bool
S9sRpcReply::printGraph()
{
    S9sOptions      *options       = S9sOptions::instance();
    int              terminalWidth = options->terminalWidth();
    int              clusterId     = options->clusterId();
    S9sVariantList   hostList      = operator[]("hosts").toVariantList();
    bool             success       = false;
    S9sVector<S9sCmonGraph *> graphs;

    S9S_DEBUG("Printing graphs.");
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return true;
    }

    /*
     * Going through the hosts, creating graphs for them.
     */
    for (uint idx = 0u; idx < hostList.size(); ++idx)
    {
        S9sVariantMap hostMap = hostList[idx].toVariantMap();
        S9sNode       host    = hostMap;

        // Filtering...
        S9S_DEBUG("  host : %s", STR(host.hostName()));
        if (!options->isStringMatchExtraArguments(host.hostName()))
        {
            S9S_DEBUG("  filtered...");
            continue;
        }

        // This should be filtered by the controller, but it might not. Well, we
        // had a problem with this.
        if (clusterId != host.clusterId())
        {
            S9S_DEBUG("  in other cluster...");
            continue;
        }

        //printf("h: %s id: %d\n", STR(host.hostName()), host.id());
        success = createGraph(graphs, host);
        if (!success)
            break;
    }


    int sumWidth = 0; 
    int nPrinted = 0;
    S9sVector<S9sGraph *> selectedGraphs;
    S9sString columnSeparator = "  ";

    for (uint idx = 0u; idx < graphs.size(); ++idx)
    {
        S9sCmonGraph *graph = graphs[idx];
        int           thisWidth = graph->nColumns();
        int           separatorWidth = 0;

        if (sumWidth > 0)
            separatorWidth = columnSeparator.length();

        if (sumWidth + thisWidth + separatorWidth <= terminalWidth)
        {
            selectedGraphs << graph;
            sumWidth += thisWidth;
            sumWidth += separatorWidth;
        } else {
            if (nPrinted > 0)
                printf("\n\n");

            S9sGraph::printRow(selectedGraphs, columnSeparator);
            nPrinted += selectedGraphs.size();

            selectedGraphs.clear();
            sumWidth = 0;

            selectedGraphs << graph;
            sumWidth += thisWidth;
        }
    }

    if (!selectedGraphs.empty())
    {
        if (nPrinted > 0)
            printf("\n\n");

        S9sGraph::printRow(selectedGraphs, columnSeparator);
        nPrinted += selectedGraphs.size();
    }

    if (nPrinted > 0)
        printf("\n");

    /*
     * Destroying the graph objects.
     */
    for (uint idx = 0u; idx < graphs.size(); ++idx)
    {
        S9sCmonGraph *graph = graphs[idx];

        delete graph;
    }

    return success;
}

/**
 * Prints one host in the "stat" format, the format that print every single
 * detail. Well, a lot of details anyway.
 */
void
S9sRpcReply::printNodeStat(
        S9sCluster &cluster,
        S9sNode    &node)
{
    S9sOptions *options = S9sOptions::instance();
    int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    S9sString   title;
    S9sString   slavesAsString;
    S9sString   message;
    
    //
    // The title line that is in inverse. 
    //
    if (node.port() > 0)
        title.sprintf(" %s:%d ", STR(node.name()), node.port());
    else
        title.sprintf(" %s ", STR(node.name()));

    printf("%s", TERM_INVERSE/*headerColorBegin()*/);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL /*headerColorEnd()*/);
    printf("\n");

    //
    // "    Name: 192.168.1.127           Cluster: galera_001 (1)"
    //
    printf("%s    Name:%s ", greyBegin, greyEnd);
    printf("%-16s ", STR(node.name()));
    //printf("\n");
    
    printf("%s       Cluster:%s ", greyBegin, greyEnd);
    printf("%s%s%s (%d) ", 
            clusterColorBegin(), 
            STR(cluster.name()), 
            clusterColorEnd(),
            cluster.clusterId());
    printf("\n");
   
    //
    //
    //
    printf("%s      IP:%s ", greyBegin, greyEnd);
    printf("%-16s ", STR(node.ipAddress()));
    //printf("\n");
    
    printf("          %sPort:%s ", greyBegin, greyEnd);
    if (node.hasPort())
    printf("%d ", node.port());
    printf("\n");
    
    // 
    // Line 
    //
    printf("%s   Alias:%s ", greyBegin, greyEnd);
    printf("%-25s", STR(node.alias("-")));
    //printf("\n");
    
    printf("%s Owner:%s ", greyBegin, greyEnd);
    printf("%s%s%s/%s%s%s ", 
            userColorBegin(), STR(node.ownerName()), userColorEnd(),
            groupColorBegin(node.groupOwnerName()), 
            STR(node.groupOwnerName()), 
            groupColorEnd());
    printf("\n");
   
    //
    // "   Class: CmonPostgreSqlHost         Type: postgres"
    //
    printf("%s   Class:%s ", greyBegin, greyEnd);
    printf("%s%-24s%s ", 
            typeColorBegin(), 
            STR(node.className()), 
            typeColorEnd());
    
    printf("%s  Type:%s ", greyBegin, greyEnd);
    printf("%s", STR(node.nodeType()));
    printf("\n");
   
    //
    //
    //
    printf("%s  Status:%s ", greyBegin, greyEnd);
    printf("%-24s", STR(node.hostStatus()));
    //printf("\n");
    
    printf("   %sRole:%s ", greyBegin, greyEnd);
    printf("%s", STR(node.role()));
    printf("\n");
    
    
    printf("%s      OS:%s ", greyBegin, greyEnd);
    printf("%-24s", STR(node.osVersionString()));

    printf("%s Access:%s ", greyBegin, greyEnd);
    printf("%s", node.readOnly() ? "read-only" : "read-write");

    printf("\n");
    
    //
    printf("%s   VM ID:%s ", greyBegin, greyEnd);
    printf("%s", STR(node.containerId("-")));
    printf("\n");

    //  Version: 1.4.7
    printf("%s Version:%s ", greyBegin, greyEnd);
    printf("%s", STR(node.version()));
    printf("\n");


    // A line for the human readable message.
    message = node.message();
    if (message.empty())
        message = "-";
    printf("%s Message:%s ", greyBegin, greyEnd);
    printf("%s", STR(message));
    printf("\n");
   
    slavesAsString = node.slavesAsString();
    if (!slavesAsString.empty())
    {
        printf("%s  Slaves:%s ", greyBegin, greyEnd);
        printf("%s", STR(slavesAsString));
        printf("\n");
    }

    /*
     * Last seen time and SSH fail count.
     */
    printf("%sLastSeen:%s ", greyBegin, greyEnd);
    printf("%-27s", STR(S9sString::pastTime(node.lastSeen())));
    //printf("\n");
    
    printf("%s SSH:%s ", greyBegin, greyEnd);
    printf("%d ", node.sshFailCount());
    printf("%sfail(s)%s ", greyBegin, greyEnd);

    printf("\n");

    //
    // A line of switches.
    //
    printf("%s Connect:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.readOnly()));

    printf("%sMaintenance:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.isMaintenanceActive()));
    
    printf("%sManaged:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.managed()));
    
    printf("%sRecovery:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.nodeAutoRecovery()));

    printf("%sSkip DNS:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.skipNameResolve()));
    
    printf("%sSuperReadOnly:%s %c ", 
            greyBegin, greyEnd, 
            BoolToHuman(node.superReadOnly()));

    
    printf("\n");
    
    //
    //
    //
    if (node.pid() > 0)
    {
        printf("%s     Pid:%s %d", 
                greyBegin, greyEnd, 
                node.pid());
    } else {
        printf("%s     PID:%s -", greyBegin, greyEnd);
    }
    
    printf("  %sUptime:%s %s", 
            greyBegin, greyEnd, 
            STR(S9sString::uptime(node.uptime())));
    
    printf("\n");

    //
    // Lines of various files.
    //
    printf("%s  Config:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(node.configFile()),
            STR(node.configFile()),
            fileColorEnd());
    printf("\n");
    
    printf("%s LogFile:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(node.logFile()),
            STR(node.logFile()),
            fileColorEnd());
    printf("\n");

    printf("%s PidFile:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(node.pidFile()),
            STR(node.pidFile()),
            fileColorEnd());
    printf("\n");
    
    printf("%s DataDir:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            XTERM_COLOR_BLUE,
            STR(node.dataDir()),
            TERM_NORMAL);

    printf("\n");

    printBackendServersSubList(node);
}

void
S9sRpcReply::printBackendServersSubList(
        const S9sNode &node)
{
    if (node.hasBackendServers())
    {
        S9sOptions    *options = S9sOptions::instance();
        int            terminalWidth = options->terminalWidth();
        S9sFormat      hostNameFormat(ipColorBegin(), ipColorEnd());
        S9sFormat      portFormat;
        S9sFormat      statusFormat;
        S9sFormat      commentFormat;
        int            tableWidth;
        S9sString      indent;

        /*
         *
         */
        hostNameFormat.widen("NAME");
        portFormat.widen("PORT");
        statusFormat.widen("STATUS");
        commentFormat.widen("COMMENT");
        
        for (uint idx = 0u; idx < node.numberOfBackendServers(); ++idx)
        {
            S9sString hostName = node.backendServerName(idx);
            int       port     = node.backendServerPort(idx);
            S9sString status   = node.backendServerStatus(idx);
            S9sString comment  = node.backendServerComment(idx);

            hostNameFormat.widen(hostName);
            portFormat.widen(port);
            statusFormat.widen(status);
            commentFormat.widen(comment);
        }

        tableWidth = 3 +
            hostNameFormat.realWidth() + portFormat.realWidth() +
            statusFormat.realWidth()   + commentFormat.realWidth();
    
        if (terminalWidth - tableWidth > 0)
            indent = S9sString(" ") * ((terminalWidth - tableWidth) / 2);

        printf("\n");
        
        printf("%s", headerColorBegin());
        printf("%s", STR(indent));
        hostNameFormat.printf("NAME", false);
        portFormat.printf("PORT", false);
        statusFormat.printf("STATUS", false);
        commentFormat.printf("COMMENT", false);
        printf("%s", headerColorEnd());
        printf("\n");

        for (uint idx = 0u; idx < node.numberOfBackendServers(); ++idx)
        {
            S9sString hostName = node.backendServerName(idx);
            int       port     = node.backendServerPort(idx);
            S9sString status   = node.backendServerStatus(idx);
            S9sString comment  = node.backendServerComment(idx);

            printf("%s", STR(indent));
            hostNameFormat.printf(hostName);
            portFormat.printf(port);
            statusFormat.printf(status);
            commentFormat.printf(comment);

            printf("\n");
        }
            
        printf("\n");
    }
}

void 
S9sRpcReply::printNodeListStat()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = clusters();

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  clusterMap  = theList[idx].toVariantMap();
        S9sCluster     cluster     = clusterMap;
        S9sVariantList hosts       = clusterMap["hosts"].toVariantList();

        for (uint idx2 = 0; idx2 < hosts.size(); ++idx2)
        {
            S9sVariantMap hostMap   = hosts[idx2].toVariantMap();
            S9sNode       node      = hostMap;
           
            if (!options->isStringMatchExtraArguments(node.name()))
                continue;
            
            printNodeStat(cluster, node);
        }
    }
}

void 
S9sRpcReply::printObjectStat(
        S9sObject    &object)
{
    //S9sOptions *options = S9sOptions::instance();
    //int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    
    //
    // "   Name: www"
    //
    printf("%s    Name:%s ", greyBegin, greyEnd);
    printf("%s", clusterColorBegin());
    printf("%-32s ", STR(object.name()));
    printf("%s", clusterColorEnd());
    
    printf("\n");
   
    //
    //
    //
    printf("%sCDT path:%s ", greyBegin, greyEnd);
    printf("%s", folderColorBegin());
    printf("%-32s ", STR(object.cdtPath()));
    printf("%s", clusterColorEnd());
    printf("\n");
    
    //
    //
    //
    printf("%s   Class:%s ", greyBegin, greyEnd);
    printf("%s%-33s%s ", 
            typeColorBegin(), 
            STR(object.className()), 
            typeColorEnd());
    
    printf("%s   Owner:%s ", greyBegin, greyEnd);
    printf("%s%s%s/%s%s%s ", 
            userColorBegin(), STR(object.ownerName()), userColorEnd(),
            groupColorBegin(object.groupOwnerName()), 
            STR(object.groupOwnerName()), 
            groupColorEnd());
    
    printf("\n");
    
    //
    //
    //
    printf("%s      ID:%s ", greyBegin, greyEnd);
    printf("%-38s", STR(object.id("-")));

    printf("%s ACL:%s ", greyBegin, greyEnd);
    printf("%s", STR(object.aclShortString()));

#if 0
    printf("\n");
    printf(
        "--------------------------------------------------------------------"
        "------------");
#endif
    printf("\n");
}

void 
S9sRpcReply::printContainerStat(
        S9sContainer &container)
{
    S9sOptions *options = S9sOptions::instance();
    S9s::AddressType addressType = options->addressType();
    int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    S9sString   title;
    S9sString   tmpString;
    S9sString   tmp;

    //
    // The title that is in inverse. 
    //
    if (container.ipAddress(addressType).empty())
    {
        title.sprintf(" %s ", STR(container.alias()));
    } else {
        title.sprintf(" %s (%s)", 
                STR(container.alias()), 
                STR(container.ipAddress(addressType)));
    }

    printf("%s", TERM_INVERSE);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL);
    
    printObjectStat(container);


    //
    // "    IPv4: 54.93.99.244                          Type: cmon-cloud"
    //
    printf("%s    IPv4:%s ", greyBegin, greyEnd);
    printf("%-37s", STR(container.ipv4Addresses()));

    printf("%s Type:%s ", greyBegin, greyEnd);
    printf("%s", STR(container.type()));

    printf("\n");
    
    //
    //
    //
    tmp = container.ipAddress(S9s::PublicIpv4Address, "-");
    printf("%sPublicIp:%s ", greyBegin, greyEnd);
    printf("%s%-33s%s", ipColorBegin(tmp), STR(tmp), ipColorEnd(tmp));

    tmp = container.ipAddress(S9s::PrivateIpv4Address, "-");
    printf("%sPrivateIp:%s ", greyBegin, greyEnd);
    printf("%s%s%s", ipColorBegin(tmp), STR(tmp), ipColorEnd(tmp));

    printf("\n");
     
    //
    // "   Cloud: az                                  Region: Southeast Asia"
    //
    printf("%s   Cloud:%s ", greyBegin, greyEnd);
    printf("%-34s", STR(container.provider("-")));

    printf("%s  Region:%s ", greyBegin, greyEnd);
    printf("%s", STR(container.region("-")));

    printf("\n");

    //
    // "  Subnet: subnet-6a1d1c12                       CIDR: 172.31.0.0/20"
    //
    printf("%s  Subnet:%s ", greyBegin, greyEnd);
    printf("%-34s", STR(container.subnetId("-")));
    
    printf("%s    CIDR:%s ", greyBegin, greyEnd);
    printf("%s", STR(container.subnetCidr("-")));

    printf("\n");
    
    //
    //
    //
    printf("%s  VPC ID:%s ", greyBegin, greyEnd);
    printf("%-34s", STR(container.subnetVpcId("-")));

    printf("\n");
    
    //
    //
    //
    printf("%sFirewall:%s ", greyBegin, greyEnd);
    printf("%-34s", STR(container.firewalls("-")));

    printf("\n");

    //
    //
    //
    printf("%sTemplate:%s ", greyBegin, greyEnd);
    printf("%-36s", STR(container.templateName("-")));

    printf("%s Image:%s ", greyBegin, greyEnd);
    printf("%s", STR(container.image("-")));

    printf("\n");

    //
    // "      OS: ubuntu 16.04 xenial                  Arch: x86_64"
    //
    printf("%s      OS:%s ", greyBegin, greyEnd);
    printf("%-36s", STR(container.osVersionString()));
    
    printf("%s  Arch:%s ", greyBegin, greyEnd);
    printf("%s ", STR(container.architecture()));

    printf("\n");
    
    //
    //
    //
    printf("%s  Server:%s ", greyBegin, greyEnd);
    printf("%s", serverColorBegin());
    printf("%-33s ", STR(container.parentServerName()));
    printf("%s", serverColorEnd());
    
    printf("%s   State:%s ", greyBegin, greyEnd);
    printf("%s%s%s ", 
            clusterStateColorBegin(container.state()), 
            STR(container.state()),
            clusterStateColorEnd());

    printf("\n");

    //
    //
    //
    printf("%s   Start:%s ", greyBegin, greyEnd);
    printf("%s",  container.autoStart() ? "y" : "n");
    printf("\n");
    
    //
    //
    //
    printf("%s  Limits:%s ", greyBegin, greyEnd);

    tmpString = "";
    if (container.memoryLimitGBytes() > 0)
        tmpString.aprintf("%.0fGB RAM",  container.memoryLimitGBytes());

    for (uint idx = 0u; idx < container.nVolumes(); ++idx)
    {
        if (!tmpString.empty())
            printf(", ");
        
        ::printf("%dGB %s", 
                container.volumeGigaBytes(idx), 
                STR(container.volumeType(idx).toUpper()));
    }

    ::printf("%s", STR(tmpString));
    ::printf("\n");


    //
    // "  Config: '/var/lib/lxc/www/config'"
    //
    printf("%s  Config:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            fileColorBegin(container.configFile()),
            STR(container.configFile()),
            fileColorEnd());
    
    printf("\n");
    
    //
    //
    //
    printf("%s Root FS:%s ", greyBegin, greyEnd);
    printf("'%s%s%s'", 
            XTERM_COLOR_BLUE,
            STR(container.rootFsPath()),
            TERM_NORMAL);

    printf("\n");
    printf("\n");
    printf("\n");
}


void 
S9sRpcReply::printContainerStat()
{
    S9sOptions     *options   = S9sOptions::instance();
    S9sString       subnetId  = options->subnetId();
    S9sString       vpcId     = options->vpcId();
    S9sString       cloudName = options->cloudName();
    S9sVariantList  theList   = operator[]("containers").toVariantList();
    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sContainer   container(theMap);

        if (!cloudName.empty() && container.provider() != cloudName)
            continue;

        if (!subnetId.empty() && container.subnetId() != subnetId)
            continue;
        
        if (!vpcId.empty() && vpcId != container.subnetVpcId())
            continue;

        if (!options->isStringMatchExtraArguments(container.name()))
            continue;


        printContainerStat(container);
    }
}

void 
S9sRpcReply::printClusterListStat()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = clusters();

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  clusterMap  = theList[idx].toVariantMap();
        S9sCluster     cluster     = clusterMap;
        S9sVariantList hosts       = clusterMap["hosts"].toVariantList();
     
        if (!options->isStringMatchExtraArguments(cluster.name()))
            continue;

        printClusterStat(cluster);
    }
}

/**
 * Prints the node list in its long format (aka "node --list --long).
 */
void 
S9sRpcReply::printNodeListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantMap   properties = options->propertiesOption();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       clusterNameFilter = options->clusterName();
    S9sVariantList  theList = clusters();
    S9sString       formatString = options->longNodeFormat();
    S9sVariantList  hostList;
    S9sFormat       cidFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       versionFormat;
    S9sFormat       clusterNameFormat;
    S9sFormat       portFormat;
    int             total = 0;
    int             terminalWidth = options->terminalWidth();
    int             nColumns;

    if (options->hasNodeFormat())
        formatString = options->nodeFormat();

    /*
     * If there is a format string we simply print the list using that format.
     */
    if (!formatString.empty())
    {
        for (uint idx = 0; idx < theList.size(); ++idx)
        {
            S9sVariantMap  theMap      = theList[idx].toVariantMap();
            S9sVariantList hosts       = theMap["hosts"].toVariantList();
            S9sString      clusterName = theMap["cluster_name"].toString();

            total += hosts.size();
   
            if (!clusterNameFilter.empty() && clusterNameFilter != clusterName)
                continue;

            for (uint idx2 = 0; idx2 < hosts.size(); ++idx2)
            {
                S9sVariantMap hostMap   = hosts[idx2].toVariantMap();
                S9sNode       node      = hostMap;
                int           clusterId = node.clusterId();
                S9sCluster    cluster   = clusterMap(clusterId);
                S9sString     hostName  = node.name();

                if (!properties.isSubSet(hostMap))
                    continue;

                if (!options->isStringMatchExtraArguments(hostName))
                    continue;

                node.setCluster(cluster);

                printf("%s", STR(node.toString(syntaxHighlight, formatString)));
            }
        }
    
        if (!options->isBatchRequested())
            printf("Total: %d\n", total); 

        return;
    }

    /*
     * First run-through: collecting some information.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap      = theList[idx].toVariantMap();
        S9sVariantList hosts       = theMap["hosts"].toVariantList();
        S9sString      clusterName = theMap["cluster_name"].toString();

        total += hosts.size();

        if (!clusterNameFilter.empty() && clusterNameFilter != clusterName)
            continue;

        clusterNameFormat.widen(clusterName);

        for (uint idx2 = 0; idx2 < hosts.size(); ++idx2)
        {
            S9sVariantMap hostMap   = hosts[idx2].toVariantMap();
            S9sNode       node      = hostMap;
            S9sString     hostName  = node.name();
            S9sString     version   = node.version();
            int           port      = hostMap["port"].toInt(-1);
            int           clusterId = hostMap["clusterid"].toInt();

            if (!properties.isSubSet(hostMap))
                continue;

            if (!options->isStringMatchExtraArguments(hostName))
                continue;

            if (version.empty())
                version = "-";
            
            hostNameFormat.widen(hostName);
            cidFormat.widen(clusterId);
            versionFormat.widen(version);
            portFormat.widen(port);

            hostMap["cluster_name"] = clusterName;
            hostList << hostMap;
        }
    }

    /*
     * Sorting the hosts.
     */
    sort(hostList.begin(), hostList.end(), compareHostMaps);
    
    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        versionFormat.widen("VERSION");
        cidFormat.widen("CID");
        clusterNameFormat.widen("CLUSTER");
        hostNameFormat.widen("HOST");
        portFormat.widen("PORT");

        printf("%s", headerColorBegin());
        printf("STAT ");
        versionFormat.printf("VERSION");
        cidFormat.printf("CID");
        clusterNameFormat.printf("CLUSTER");
        hostNameFormat.printf("HOST");
        portFormat.printf("PORT");
        printf("COMMENT");
        printf("%s\n", headerColorEnd());
    }
        
    /*
     * Second run: doing the actual printing.
     */
    for (uint idx2 = 0; idx2 < hostList.size(); ++idx2)
    {
        S9sVariantMap hostMap   = hostList[idx2].toVariantMap();
        S9sNode       node      = hostMap;
        S9sString     hostName  = node.name();
        int           clusterId = node.clusterId();
        S9sString     status    = node.hostStatus();
        S9sString     className = hostMap["class_name"].toString();
        //S9sString     nodeType  = hostMap["nodetype"].toString();
        S9sString     message   = node.message();
        S9sString     version   = node.version();
        S9sString     clusterName = hostMap["cluster_name"].toString();
        bool maintenance = hostMap["maintenance_mode_active"].toBoolean();
        int           port      = hostMap["port"].toInt(-1);
            
        // Filtering...
        if (!properties.isSubSet(hostMap))
            continue;

        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        if (message.empty())
            message = "-";

        if (version.empty())
            version = "-";

        // FIXME: I am not sure this is actually user friendly.
        if (syntaxHighlight)
        {
            if (status == "CmonHostRecovery" ||
                    status == "CmonHostShutDown")
            {
                hostNameFormat.setColor(XTERM_COLOR_YELLOW, TERM_NORMAL);
            } else if (status == "CmonHostUnknown" ||
                    status == "CmonHostOffLine")
            {
                hostNameFormat.setColor(XTERM_COLOR_RED, TERM_NORMAL);
            } else {
                hostNameFormat.setColor(XTERM_COLOR_GREEN, TERM_NORMAL);
            }
        }

        // Calculating how much space we have for the message column.
        nColumns  = 3 + 1 + 1;
        nColumns += cidFormat.realWidth();
        nColumns += versionFormat.realWidth();
        nColumns += clusterNameFormat.realWidth();
        nColumns += hostNameFormat.realWidth();
        nColumns += portFormat.realWidth();

        if (nColumns < terminalWidth)
        {
            int remaining = terminalWidth - nColumns;
            
            if (remaining < (int) message.length())
            {
                message.resize(remaining - 1);
                message += "…";
            }
        }

        /*
         * Printing.
         */
        printf("%c", node.nodeTypeFlag());
        printf("%c", node.hostStatusFlag());
        printf("%c", node.roleFlag());
        printf("%c ", maintenance ? 'M' : '-');

        versionFormat.printf(version);
        cidFormat.printf(clusterId);

        printf("%s", clusterColorBegin());
        clusterNameFormat.printf(clusterName);
        printf("%s", clusterColorEnd());

        hostNameFormat.printf(hostName);

        if (port >= 0)
            portFormat.printf(port);
        else
            portFormat.printf("-");


        printf("%s\n", STR(message));
    }

    if (!options->isBatchRequested())
        printf("Total: %d\n", total); 
}

/**
 * Prints the job list in the brief format. Well, a brief format for the jobs is
 * still a long list, one job in every line.
 */
void 
S9sRpcReply::printJobListBrief()
{
    S9sOptions     *options         = S9sOptions::instance();
    S9sVariantList  theList         = jobs();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  requiredTags    = options->jobTags();
    int             total           = operator[]("total").toInt();
    S9sFormat       idFormat;
    S9sFormat       cidFormat;
    S9sFormat       stateFormat;
    S9sFormat       userFormat;
    S9sFormat       groupFormat;
    S9sFormat       dateFormat;
    S9sFormat       percentFormat;

    theList.reverse();

    //
    // First run, collecting some information. 
    //
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sJob         job    = theList[idx].toVariantMap();
        int            jobId  = job.id();
        int            cid    = job.clusterId();
        S9sString      user   = job.userName();
        S9sString      group  = job.groupName();
        S9sString      status = job.status();
        S9sDateTime    timeStamp;
        S9sString      timeStampString;
        
        // Filtering.
        if (options->hasJobId() && options->jobId() != jobId)
            continue;

        if (group.empty())
            group = "-";

        if (!job.hasTags(requiredTags))
            continue;

        // The timestamp. Now we use 'created' later we can make this
        // configurable.
        timeStamp.parse(job.createdString());
        timeStampString = options->formatDateTime(timeStamp);
        
        idFormat.widen(jobId);
        cidFormat.widen(cid);
        stateFormat.widen(status);
        userFormat.widen(user);
        groupFormat.widen(group);
        dateFormat.widen(timeStampString);
    }

    //
    // Printing the header.
    //
    if (!options->isNoHeaderRequested())
    {
        idFormat.widen("ID");
        cidFormat.widen("CID");
        stateFormat.widen("STATE");
        userFormat.widen("OWNER");
        groupFormat.widen("GROUP");
        dateFormat.widen("CREATED");
        percentFormat.widen("100%");

        printf("%s", headerColorBegin());
        idFormat.printf("ID");
        cidFormat.printf("CID");
        stateFormat.printf("STATE");
        userFormat.printf("OWNER");
        groupFormat.printf("GROUP");
        dateFormat.printf("CREATED");
        percentFormat.printf("RDY");
        printf("TITLE");

        printf("%s", headerColorEnd());

        printf("\n");
    }

    //
    // Second run, doing the actual printing.
    //
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sJob         job    = theList[idx].toVariantMap();
        int            jobId  = job.id();
        int            cid    = job.clusterId();
        S9sString      status = job.status();
        S9sString      title  = job.title();
        S9sString      user   = job.userName();
        S9sString      group  = job.groupName();
        S9sString      percent;
        S9sDateTime    created;
        S9sString      timeStamp;
        const char    *stateColorStart = "";
        const char    *stateColorEnd   = "";
        
        // Filtering.
        if (options->hasJobId() && options->jobId() != jobId)
            continue;

        // The title.
        if (title.empty())
            title = "Untitled Job";

        // The user name or if it is not there the user ID.
        if (user.empty())
            user.sprintf("%d", theMap["user_id"].toInt());
        
        if (group.empty())
            group = "-";
        
        if (!job.hasTags(requiredTags))
            continue;

        // The progress.
        if (job.hasProgressPercent())
        {
            percent.sprintf("%3.0f%%", job.progressPercent());
        } else if (status == "FINISHED") 
        {
            percent = "100%";
        } else {
            percent = "  0%";
        }

        // The timestamp.
        created.parse(theMap["created"].toString());
        timeStamp = options->formatDateTime(created);

        if (syntaxHighlight)
        {
            if (status.startsWith("RUNNING"))
            {
                stateColorStart = XTERM_COLOR_GREEN;
                stateColorEnd   = TERM_NORMAL;
            } else if (status == "FINISHED")
            {
                stateColorStart = XTERM_COLOR_GREEN;
                stateColorEnd   = TERM_NORMAL;
            } else if (status == "FAILED")
            {
                stateColorStart = XTERM_COLOR_RED;
                stateColorEnd   = TERM_NORMAL;
            }
        }

        idFormat.printf(jobId);
        cidFormat.printf(cid);

        printf("%s", stateColorStart);
        stateFormat.printf(status);
        printf("%s", stateColorEnd);
        
        printf("%s", userColorBegin());
        userFormat.printf(user);
        printf("%s", userColorEnd());

        printf("%s", groupColorBegin(group));
        groupFormat.printf(group);
        printf("%s", groupColorEnd());

        dateFormat.printf(timeStamp);
        percentFormat.printf(percent);
        printf("%s\n", STR(title));
    }
    
    if (!options->isBatchRequested())
        printf("Total: %d\n", total);
}

/**
 * Prints the list of jobs in their detailed form.
 *
--------------------------------------------------------------------------------
Setup PostgreSQL Server
Job finished.                                                      [██████████] 
                                                                      100.00% 
Created   : 2017-03-17 15:37:47    ID   : 84         Status : FINISHED 
Started   : 2017-03-17 15:37:52    User : pipas      Host   :  
Ended     : 2017-03-17 15:39:18    Group: users      
--------------------------------------------------------------------------------

    {
        "can_be_aborted": false,
        "can_be_deleted": true,
        "class_name": "CmonJobInstance",
        "cluster_id": 0,
        "created": "2017-03-17T14:37:47.000Z",
        "ended": "2017-03-17T14:39:18.000Z",
        "exit_code": 0,
        "group_id": 2,
        "group_name": "users",
        "has_progress": true,
        "job_id": 84,
        "job_spec": 
        {
            "command": "setup_server",
            "job_data": 
            {
                "cluster_name": "ft_postgresql_3680",
                "cluster_type": "postgresql_single",
                "enable_uninstall": true,
                "hostname": "192.168.1.117",
                "postgre_password": "passwd12",
                "postgre_user": "postmaster",
                "ssh_user": "pipas",
                "type": "postgresql"
            }
        },
        "progress_percent": 100,
        "started": "2017-03-17T14:37:52.000Z",
        "status": "FINISHED",
        "status_text": "Job finished.",
        "title": "Setup PostgreSQL Server",
        "user_id": 3,
        "user_name": "pipas"
    },
 */
void 
S9sRpcReply::printJobListLong()
{
    S9sOptions     *options         = S9sOptions::instance();
    int             terminalWidth   = options->terminalWidth();
    S9sVariantList  theList         = jobs();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  requiredTags    = options->jobTags();
    int             total           = operator[]("total").toInt();
    unsigned int    userNameLength  = 0;
    S9sString       userNameFormat;
    unsigned int    statusLength    = 0;
    S9sString       statusFormat;

    theList.reverse();

    //
    // The width of certain columns are variable.
    //
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap     = theList[idx].toVariantMap();
        S9sJob         job        = theList[idx].toVariantMap();
        int            jobId      = theMap["job_id"].toInt();
        S9sString      user       = theMap["user_name"].toString();
        S9sString      status     = theMap["status"].toString();
        
        // Filtering.
        if (options->hasJobId() && options->jobId() != jobId)
            continue;

        if (!job.hasTags(requiredTags))
            continue;

        if (user.length() > userNameLength)
            userNameLength = user.length();
        
        if (status.length() > statusLength)
            statusLength = status.length();
    }

    userNameFormat.sprintf("%%-%us ", userNameLength);
    statusFormat.sprintf("%%s%%-%ds%%s ", statusLength);

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap     = theList[idx].toVariantMap();
        S9sJob         job        = theList[idx].toVariantMap();
        int            jobId      = job.id();
        S9sString      status     = job.status();
        S9sString      title      = job.title();
        S9sString      statusText = theMap["status_text"].toString();
        S9sString      statusTextMonochrome;
        S9sString      user       = theMap["user_name"].toString();
        S9sString      group      = theMap["group_name"].toString();
        S9sString      hostName   = theMap["ip_address"].toString();
        S9sString      created    = theMap["created"].toString();
        S9sString      ended      = theMap["ended"].toString();
        S9sString      started    = theMap["started"].toString();
        S9sString      scheduled  = theMap["scheduled"].toString();
        int            clusterId  = theMap["cluster_id"].toInt();
        S9sString      bar;
        double         percent;
        S9sString      timeStamp;
        const char    *stateColorStart = "";
        const char    *stateColorEnd   = "";

        // Filtering.
        if (options->hasJobId() && options->jobId() != jobId)
            continue;

        if (!job.hasTags(requiredTags))
            continue;

        // The title.
        if (title.empty())
            title = "Untitled Job";

        // The host.
        if (hostName.empty())
            hostName = "-";

        // Status text
        statusTextMonochrome = S9sString::html2text(statusText);
        statusText = S9sString::html2ansi(statusText);

        // The user name or if it is not there the user ID.
        if (user.empty())
            user.sprintf("%d", theMap["user_id"].toInt());

        // The progress.
        if (theMap.contains("progress_percent"))
        {
            percent = theMap["progress_percent"].toDouble();
        } else if (status == "FINISHED") 
        {
            percent = 100.0;
        } else {
            percent = 0.0;
        }

        /*
         * The timestamps.
         */
        if (!created.empty())
        {
            S9sDateTime tmp;

            tmp.parse(created);
            created = tmp.toString(S9sDateTime::MySqlLogFileFormat);
        }
        
        if (!scheduled.empty())
        {
            S9sDateTime tmp;

            tmp.parse(scheduled);
            scheduled = tmp.toString(S9sDateTime::MySqlLogFileFormat);
        }
        
        if (!started.empty())
        {
            S9sDateTime tmp;

            tmp.parse(started);
            started = tmp.toString(S9sDateTime::MySqlLogFileFormat);
        }
        
        if (!ended.empty())
        {
            S9sDateTime tmp;

            tmp.parse(ended);
            ended = tmp.toString(S9sDateTime::MySqlLogFileFormat);
        }

        //
        if (syntaxHighlight)
        {
            if (status.startsWith("RUNNING"))
            {
                stateColorStart = XTERM_COLOR_GREEN;
                stateColorEnd   = TERM_NORMAL;
            } else if (status == "FINISHED")
            {
                stateColorStart = XTERM_COLOR_GREEN;
                stateColorEnd   = TERM_NORMAL;
            } else if (status == "FAILED")
            {
                stateColorStart = XTERM_COLOR_RED;
                stateColorEnd   = TERM_NORMAL;
            }
        }

        /*
         * A line, then a title and a status text.
         */
        for (int n = 0; n < terminalWidth; ++n)
            printf("-");

        printf("\n");

        printf("%s%s%s\n", TERM_BOLD, STR(title), TERM_NORMAL);
        printf("%s", STR(statusText));

        for (int n = statusTextMonochrome.length(); n < terminalWidth - 13; ++n)
            printf(" ");

        if (theMap.contains("progress_percent"))
        {
            percent = theMap["progress_percent"].toDouble();
            bar = progressBar(percent, syntaxHighlight);
        } else {
            bar = "            ";
        }

        printf("%s", STR(bar));
        printf("\n");

        for (int n = 11; n < terminalWidth; ++n)
            printf(" ");

        if (theMap.contains("progress_percent"))
            printf("%6.2f%% ", percent);
        else 
            printf("        ");


        //printf(STR(userNameFormat), STR(user));
        //printf("%s ", STR(timeStamp));
        //printf("%s ", STR(percent));
        //printf("%5d ", jobId);
        printf("\n");
        
        // The dates...
        printf("%sCreated   :%s %s%19s%s    ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_LIGHT_GRAY, STR(created), TERM_NORMAL);

        printf("%sID   :%s %s%-10d%s ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_BLUE, jobId, TERM_NORMAL);

        printf("%sStatus :%s %s%s%s ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                stateColorStart, STR(status), stateColorEnd);

        printf("\n");

        // Started : 2017-02-06 11:13:04  User : system   Host   : 127.0.0.1
        printf("%sStarted   :%s %s%19s%s    ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_LIGHT_GRAY, STR(started), TERM_NORMAL);
        
        printf("%sUser :%s %s%-10s%s ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                userColorBegin(), STR(user), userColorEnd());

        printf("%sHost   :%s %s%s%s ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_BLUE, STR(hostName), TERM_NORMAL);
        
        printf("\n");



        printf("%sEnded     :%s %s%19s%s    ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_LIGHT_GRAY, STR(ended), TERM_NORMAL);
        
        printf("%sGroup:%s %s%-10s%s ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                groupColorBegin(group), STR(group), groupColorEnd());
        
        printf("%sCluster:%s %d ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                clusterId);
        
        printf("\n");
        
        if (!scheduled.empty())
        {
            printf("%sScheduled :%s %s%19s%s\n", 
                    XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                    XTERM_COLOR_LIGHT_GRAY, STR(scheduled), TERM_NORMAL);
        }

        printf("%sTags      :%s %s\n", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                STR(job.tags(syntaxHighlight, "-")));

        S9S_UNUSED(jobId);
        S9S_UNUSED(stateColorEnd);
        S9S_UNUSED(stateColorStart);
    }
        
    for (int n = 0; n < terminalWidth; ++n)
        printf("-");

    printf("\n");
    
    if (!options->isBatchRequested())
        printf("Total: %d\n", total);
}

/**
 * Prints the job log in its short format. In this format only the messages are
 * printed.
 *

{
    "cc_timestamp": 1475228277,
    "data": [ 
    {
        "busy": 0.0482585,
        "cpuid": 7,
        "cpumhz": 2000,
        "cpumodelname": "Intel(R) Xeon(R) CPU           E5345  @ 2.33GHz",
        "cpuphysicalid": 1,
        "cputemp": 55.5,
        "created": 1475226759,
        "hostid": 1,
        "idle": 0.930115,
        "interval": 122858,
        "iowait": 0.0216267,
        "loadavg1": 0.83,
        "loadavg15": 0.26,
        "loadavg5": 0.58,
        "sampleends": 1475226879,
        "samplekey": "CmonCpuStats-1-7",
        "steal": 0,
        "sys": 0.0213263,
        "uptime": 314,
        "user": 0.0269322
    }, 
    {
        "busy": 0.0132064,
        "cpuid": 7,
        "cpumhz": 2000,
        "cpumodelname": "Intel(R) Xeon(R) CPU           E5345  @ 2.33GHz",
        "cpuphysicalid": 1,
        "cputemp": 54.5,
        "created": 1475228200,
        "hostid": 4,
        "idle": 0.986709,
        "interval": 120157,
        "iowait": 8.42886e-05,
        "loadavg1": 0.04,
        "loadavg15": 0.17,
        "loadavg5": 0.1,
        "sampleends": 1475228261,
        "samplekey": "CmonCpuStats-4-7",
        "steal": 0,
        "sys": 0.0100287,
        "uptime": 1675,
        "user": 0.00317769
    } ],
    "requestStatus": "ok",
    "total": 392

 */
void
S9sRpcReply::printCpuStat()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("data").toVariantList();
    S9sVariantMap   listMap;

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap  = theList[idx].toVariantMap();
        S9sString     key     = theMap["samplekey"].toString();
        ulonglong     created = theMap["created"].toULongLong();

        if (!listMap.contains(key))
        {
            listMap[key] = theMap;
            continue;
        }

        if (listMap[key]["created"].toULongLong() < created)
            listMap[key] = theMap;
    }

    foreach (const S9sVariant variant, listMap)
    {
        S9sVariantMap theMap  = variant.toVariantMap();
        S9sString     model   = theMap["cpumodelname"].toString();
        int           id      = theMap["cpuid"].toInt();
        int           hostId  = theMap["hostid"].toInt();
        S9sString     key     = theMap["samplekey"].toString();
        double        user    = theMap["user"].toDouble() * 100.0;
        double        sys     = theMap["sys"].toDouble()  * 100.0;
        double        idle    = theMap["idle"].toDouble() * 100.0;
        double        wait    = theMap["iowait"].toDouble() * 100.0;
        double        steal   = theMap["steal"].toDouble() * 100.0;
        const char    *numberStart = "";
        const char    *numberEnd   = "";

        while (model.contains("  "))
            model.replace("  ", " ");

        if (syntaxHighlight)
        {
            numberStart = TERM_BOLD;
            numberEnd   = TERM_NORMAL;
        }

        printf("%%cpu%02d-%02d ", hostId, id);
        printf("%s%5.1f%s us,", numberStart, user, numberEnd);
        printf("%s%5.1f%s sy,", numberStart, sys, numberEnd);
        printf("%s%5.1f%s id,", numberStart, idle, numberEnd);
        printf("%s%5.1f%s wa,", numberStart, wait, numberEnd);
        printf("%s%5.1f%s st,", numberStart, steal, numberEnd);
        printf("%s\n", STR(model));
    }
}

void
S9sRpcReply::printBackupListFormatString(
        const bool longFormat)
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       formatString;
    S9sVariantList  dataList;
    
    if (longFormat)
    {
        formatString = options->longBackupFormat();

        if (formatString.empty())
            formatString = options->backupFormat();
    } else {
        formatString = options->backupFormat();
    }

    // One is RPC 1.0, the other is 2.0.
    if (contains("data"))
        dataList = operator[]("data").toVariantList();
    else if (contains("backup_records"))
        dataList = operator[]("backup_records").toVariantList();

    S9S_DEBUG(" dataList.size(): %lu",  dataList.size());
    for (uint idx = 0; idx < dataList.size(); ++idx)
    {
        S9sVariantMap  theMap    = dataList[idx].toVariantMap();
        S9sBackup      backup    = theMap;
        int            id        = backup.id();

        /*
         * Filtering.
         */
        S9S_DEBUG("      id : %d", id);
        S9S_DEBUG("nBackups : %d", backup.nBackups());
        if (options->hasBackupId() && options->backupId() != id)
            continue;

        for (int backupIdx = 0; backupIdx < backup.nBackups(); ++backupIdx)
        {
            S9S_DEBUG("  nFiles : %d", backup.nFiles(backupIdx));
            for (int fileIdx = 0; fileIdx < backup.nFiles(backupIdx); ++fileIdx)
            {
                S9sString outString;
                    
                outString = backup.toString(
                        backupIdx, fileIdx, 
                        syntaxHighlight, formatString);

                printf("%s", STR(outString));
            }        
        }
    }
        
    if (!options->isBatchRequested() && contains("total"))
    {
        int total = operator[]("total").toInt();
        printf("Total %d\n", total);
    }
}

/**
 * Prints the list of backups in its brief format.
 */
void 
S9sRpcReply::printBackupListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  dataList;

    // One is RPC 1.0, the other is 2.0.
    if (contains("data"))
        dataList = operator[]("data").toVariantList();
    else if (contains("backup_records"))
        dataList = operator[]("backup_records").toVariantList();

    /*
     * We go through the data and print the titles. 
     */
    for (uint idx = 0; idx < dataList.size(); ++idx)
    {
        S9sVariantMap  theMap    = dataList[idx].toVariantMap();
        S9sBackup      backup    = theMap;

        /*
         * Filtering.
         */
        if (options->hasBackupId() && options->backupId() != backup.id())
            continue;

        printf("%s\n", STR(backup.title()));
    }
}

/**
 * This is the one that prints the detailed list of the backups.
 */
void 
S9sRpcReply::printBackupListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  dataList;
    S9sFormat       sizeFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       idFormat;
    S9sFormat       parentIdFormat;
    S9sFormat       cidFormat;
    S9sFormat       verifyFormat;
    S9sFormat       incrementalFormat;
    S9sFormat       stateFormat;
    S9sFormat       createdFormat;
    S9sFormat       ownerFormat;
   
    // One is RPC 1.0, the other is 2.0.
    if (contains("data"))
        dataList = operator[]("data").toVariantList();
    else if (contains("backup_records"))
        dataList = operator[]("backup_records").toVariantList();

    /*
     * Collecting some information.
     */
    for (uint idx = 0; idx < dataList.size(); ++idx)
    {
        S9sVariantMap  theMap     = dataList[idx].toVariantMap();
        S9sBackup      backup     = theMap;
        S9sString      hostName   = backup.backupHost();
        int            id         = backup.id(); 
        int            parentId   = backup.parentId();
        int            clusterId  = backup.clusterId(); 
        S9sString      verifyFlag = backup.verificationFlag();
        S9sString      owner      = backup.configOwner();
        S9sString      status     = backup.status(); 
        ulonglong      fullSize   = 0ull;
        S9sString      sizeString;
        S9sString      created;

        if (options->hasBackupId() && options->backupId() != id)
            continue;

        cidFormat.widen(clusterId);
        stateFormat.widen(status);
        hostNameFormat.widen(hostName);
        ownerFormat.widen(owner);
        verifyFormat.widen(verifyFlag);
        incrementalFormat.widen("-");

        for (int backupIdx = 0; backupIdx < backup.nBackups(); ++backupIdx)
        {
            idFormat.widen(id);
            parentIdFormat.widen(parentId);

            for (int fileIdx = 0; 
                    fileIdx < backup.nFiles(backupIdx); ++fileIdx)
            {
                ulonglong size = backup.fileSize(backupIdx, fileIdx).toUll();
                
                fullSize += size;
            }
                
            sizeString = S9sFormat::toSizeString(fullSize);
            sizeFormat.widen(sizeString);
        }
                
        created = backup.beginAsString();
        createdFormat.widen(created);
    }

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        idFormat.widen("ID");
        parentIdFormat.widen("PI");
        cidFormat.widen("CID");
        verifyFormat.widen("V");
        incrementalFormat.widen("I");
        stateFormat.widen("STATE");
        ownerFormat.widen("OWNER");
        hostNameFormat.widen("HOSTNAME");
        createdFormat.widen("CREATED");
        sizeFormat.widen("SIZE");

        printf("%s", headerColorBegin());
        idFormat.printf("ID");
        parentIdFormat.printf("PI");
        cidFormat.printf("CID");
        verifyFormat.printf("V");
        incrementalFormat.printf("I");
        stateFormat.printf("STATE");
        ownerFormat.printf("OWNER");
        hostNameFormat.printf("HOSTNAME");
        createdFormat.printf("CREATED");
        sizeFormat.printf("SIZE");
        printf("TITLE");
 
        printf("%s", headerColorEnd());
        printf("\n");
    }
    
    sizeFormat.setRightJustify();
    parentIdFormat.setRightJustify();

    /*
     * Second run, we print things here.
     */
    for (uint idx = 0; idx < dataList.size(); ++idx)
    {
        S9sVariantMap  theMap     = dataList[idx].toVariantMap();
        S9sBackup      backup     = theMap;
        S9sString      hostName   = backup.backupHost();
        int            clusterId  = backup.clusterId();
        S9sString      verifyFlag = backup.verificationFlag();
        bool           hasInc     = false;
        bool           hasNotInc  = false;
        S9sString      owner      = backup.configOwner();
        int            id         = backup.id();
        int            parentId   = backup.parentId();
        S9sString      status     = backup.status();
        S9sString      root       = backup.rootDir();
        ulonglong      fullSize   = 0ull;
        S9sString      sizeString;
        S9sString      created;

        /*
         * Filtering.
         */
        if (options->hasBackupId() && options->backupId() != id)
            continue;

        /*
         * Let's keep this for a while...
         */
#if 0
        if (backups.size() == 0u)
        {
            S9sString     path          = "-";
            S9sString     sizeString    = "-";
            S9sString     createdString = "-";
                
            idFormat.printf(id);
            cidFormat.printf(clusterId);

            printf("%s", backup.statusColorBegin(syntaxHighlight));
            stateFormat.printf(status);
            printf("%s", backup.statusColorEnd(syntaxHighlight));

            ownerFormat.printf(owner);
            hostNameFormat.printf(hostName);
            createdFormat.printf(createdString);
            sizeFormat.printf(sizeString);
            printf("%s", STR(path));
            printf("\n");

            continue;
        }
#endif
        for (int backupIdx = 0; backupIdx < backup.nBackups(); ++backupIdx)
        {
            for (int fileIdx = 0; fileIdx < backup.nFiles(backupIdx); ++fileIdx)
            {
                ulonglong size = backup.fileSize(backupIdx, fileIdx).toUll();
                bool      incremental = 
                    backup.incremental(backupIdx, fileIdx).toBoolean();

                fullSize += size;
                if (incremental)
                {
                    hasInc = true;
                } else {
                    hasNotInc = true;
                }
            }
        }

        created = backup.beginAsString();
        sizeString = S9sFormat::toSizeString(fullSize);

        idFormat.printf(id);
        if (parentId > 0)
            parentIdFormat.printf(parentId);
        else
            parentIdFormat.printf("-");

        cidFormat.printf(clusterId);
        verifyFormat.printf(verifyFlag);
        
        if (hasInc && hasNotInc)
            printf("B ");
        else if (hasInc)
            printf("I ");
        else if (hasNotInc)
            printf("F ");
        else 
            printf("- ");

        printf("%s", backup.statusColorBegin(syntaxHighlight));
        stateFormat.printf(status);
        printf("%s", backup.statusColorEnd(syntaxHighlight));

        printf("%s", userColorBegin());
        ownerFormat.printf(owner);
        printf("%s", userColorEnd());

        printf("%s", ipColorBegin());
        hostNameFormat.printf(hostName);
        printf("%s", ipColorEnd());
        
        createdFormat.printf(created);
        sizeFormat.printf(sizeString);
        printf("%s", STR(backup.title()));
        printf("\n");
    }

    /*
     * Footer.
     */
    if (!options->isBatchRequested() && contains("total"))
    {
        int total = operator[]("total").toInt();

        printf("Total %d\n", total);
    }
}

/**
 * Prints the list of backups in its brief format.
 */
void 
S9sRpcReply::printBackupListDatabasesBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  dataList;

    // One is RPC 1.0, the other is 2.0.
    if (contains("data"))
        dataList = operator[]("data").toVariantList();
    else if (contains("backup_records"))
        dataList = operator[]("backup_records").toVariantList();

    /*
     * We go through the data and print the titles. 
     */
    for (uint idx = 0; idx < dataList.size(); ++idx)
    {
        S9sVariantMap  theMap    = dataList[idx].toVariantMap();
        S9sBackup      backup    = theMap;

        /*
         * Filtering.
         */
        if (options->hasBackupId() && options->backupId() != backup.id())
            continue;

        for (int idx1 = 0; idx1 < backup.nBackups(); ++idx1)
        {
            S9sString databaseNames;
            
            databaseNames = backup.databaseNamesAsString(idx1);
            if (databaseNames.empty())
                databaseNames = "-";

            printf("%s\n", STR(databaseNames));
        }
    }
}

/**
 * This is the one that prints the detailed list of the backups.
 */
void 
S9sRpcReply::printBackupListDatabasesLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  dataList;
    S9sFormat       sizeFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       idFormat;
    S9sFormat       parentIdFormat;
    S9sFormat       cidFormat;
    S9sFormat       verifyFormat;
    S9sFormat       stateFormat;
    S9sFormat       createdFormat;
    S9sFormat       ownerFormat;
    S9sFormat       incrementalFormat;
   
    // One is RPC 1.0, the other is 2.0.
    if (contains("data"))
        dataList = operator[]("data").toVariantList();
    else if (contains("backup_records"))
        dataList = operator[]("backup_records").toVariantList();

    /*
     * Collecting some information.
     */
    for (uint idx = 0; idx < dataList.size(); ++idx)
    {
        S9sVariantMap  theMap    = dataList[idx].toVariantMap();
        S9sBackup      backup    = theMap;
        S9sVariantList backups   = theMap["backup"].toVariantList();
        S9sString      hostName  = backup.backupHost();
        int            clusterId = backup.clusterId(); 
        S9sString      verifyFlag = backup.verificationFlag();
        S9sString      owner     = backup.configOwner();
        int            id        = backup.id(); 
        int            parentId   = backup.parentId();
        S9sString      status    = backup.status(); 
        ulonglong      fullSize  = 0ull;
        S9sString      sizeString;
        S9sString      created;

        if (options->hasBackupId() && options->backupId() != id)
            continue;

        cidFormat.widen(clusterId);
        stateFormat.widen(status);
        hostNameFormat.widen(hostName);
        ownerFormat.widen(owner);
        verifyFormat.widen(verifyFlag);
        incrementalFormat.widen("-");
        
        if (backups.size() == 0u)
        {
            S9sString     sizeString    = "-";
            S9sString     createdString = "-";
            
            createdFormat.widen(createdString);
            sizeFormat.widen(sizeString);
            
            continue;
        }

        for (int backupIdx = 0; backupIdx < backup.nBackups(); ++backupIdx)
        {
            idFormat.widen(id);
            parentIdFormat.widen(parentId);

            for (int fileIdx = 0; 
                    fileIdx < backup.nFiles(backupIdx); ++fileIdx)
            {
                ulonglong size = backup.fileSize(backupIdx, fileIdx).toUll();
                
                fullSize += size;
            }
                
            sizeString = S9sFormat::toSizeString(fullSize);
            sizeFormat.widen(sizeString);
        }
                
        created = backup.beginAsString();
        createdFormat.widen(created);
    }

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        idFormat.widen("ID");
        parentIdFormat.widen("PI");
        cidFormat.widen("CID");
        verifyFormat.widen("V");
        incrementalFormat.widen("I");
        stateFormat.widen("STATE");
        ownerFormat.widen("OWNER");
        hostNameFormat.widen("HOSTNAME");
        createdFormat.widen("CREATED");
        sizeFormat.widen("SIZE");

        printf("%s", headerColorBegin());
        idFormat.printf("ID");
        parentIdFormat.printf("PI");
        cidFormat.printf("CID");
        verifyFormat.printf("V");
        incrementalFormat.printf("I");
        stateFormat.printf("STATE");
        ownerFormat.printf("OWNER");
        hostNameFormat.printf("HOSTNAME");
        createdFormat.printf("CREATED");
        sizeFormat.printf("SIZE");
        printf("DATABASES");
 
        printf("%s", headerColorEnd());
        printf("\n");
    }
    
    sizeFormat.setRightJustify();
    parentIdFormat.setRightJustify();

    /*
     * Second run, we print things here.
     */
    for (uint idx = 0; idx < dataList.size(); ++idx)
    {
        S9sVariantMap  theMap    = dataList[idx].toVariantMap();
        S9sBackup      backup    = theMap;
        S9sVariantList backups   = theMap["backup"].toVariantList();
        S9sString      hostName  = backup.backupHost();
        int            clusterId = backup.clusterId();
        S9sString      verifyFlag = backup.verificationFlag();
        S9sString      owner     = backup.configOwner();
        int            id        = backup.id();
        int            parentId   = backup.parentId();
        S9sString      status    = backup.status();
        S9sString      root      = backup.rootDir();
        ulonglong      fullSize  = 0ull;
        S9sString      sizeString;
        S9sString      created;
        bool           hasInc     = false;
        bool           hasNotInc  = false;

        /*
         * Filtering.
         */
        if (options->hasBackupId() && options->backupId() != id)
            continue;

        /*
         *
         */
        if (backups.size() == 0u)
        {
            S9sString     database      = "-";
            S9sString     sizeString    = "-";
            S9sString     createdString = "-";
                
            idFormat.printf(id);
            if (parentId > 0)
                parentIdFormat.printf(parentId);
            else
                parentIdFormat.printf("-");
            
            cidFormat.printf(clusterId);
            verifyFormat.printf(verifyFlag);
            
            // incremental
            printf("- ");

            printf("%s", backup.statusColorBegin(syntaxHighlight));
            stateFormat.printf(status);
            printf("%s", backup.statusColorEnd(syntaxHighlight));
            
            printf("%s", userColorBegin());
            ownerFormat.printf(owner);
            printf("%s", userColorEnd());

            printf("%s", ipColorBegin());
            hostNameFormat.printf(hostName);
            printf("%s", ipColorEnd());

            createdFormat.printf(createdString);
            sizeFormat.printf(sizeString);
            printf("%s", STR(database));
            printf("\n");

            continue;
        }

        for (int backupIdx = 0; backupIdx < backup.nBackups(); ++backupIdx)
        {
            S9sString databaseNames;

            for (int fileIdx = 0; fileIdx < backup.nFiles(backupIdx); ++fileIdx)
            {
                ulonglong   size = backup.fileSize(backupIdx, fileIdx).toUll();
                bool      incremental = 
                    backup.incremental(backupIdx, fileIdx).toBoolean();

                fullSize += size;
                
                if (incremental)
                {
                    hasInc = true;
                } else {
                    hasNotInc = true;
                }
            }

            created = backup.beginAsString();
            sizeString = S9sFormat::toSizeString(fullSize);

            idFormat.printf(id);
            if (parentId > 0)
                parentIdFormat.printf(parentId);
            else
                parentIdFormat.printf("-");

            cidFormat.printf(clusterId);
            verifyFormat.printf(verifyFlag);
            
            if (hasInc && hasNotInc)
                printf("B ");
            else if (hasInc)
                printf("I ");
            else if (hasNotInc)
                printf("F ");
            else 
                printf("- ");

            databaseNames = backup.databaseNamesAsString(backupIdx);
            if (databaseNames.empty())
                databaseNames = "-";

            printf("%s", backup.statusColorBegin(syntaxHighlight));
            stateFormat.printf(status);
            printf("%s", backup.statusColorEnd(syntaxHighlight));

            printf("%s", userColorBegin());
            ownerFormat.printf(owner);
            printf("%s", userColorEnd());

            printf("%s", ipColorBegin());
            hostNameFormat.printf(hostName);
            printf("%s", ipColorEnd());

            createdFormat.printf(created);
            sizeFormat.printf(sizeString);
            printf("%s", STR(databaseNames));
            printf("\n");
        }
    }

    /*
     * Footer.
     */
    if (!options->isBatchRequested() && contains("total"))
    {
        int total = operator[]("total").toInt();

        printf("Total %d\n", total);
    }
}

/**
 * Prints the list of backups in its brief format.
 */
void 
S9sRpcReply::printBackupListFilesBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  dataList;
    bool            syntaxHighlight = options->useSyntaxHighlight();
    const char     *colorBegin = "";
    const char     *colorEnd   = "";

    // One is RPC 1.0, the other is 2.0.
    if (contains("data"))
        dataList = operator[]("data").toVariantList();
    else if (contains("backup_records"))
        dataList = operator[]("backup_records").toVariantList();

    /*
     * We go through the data and print the file names. 
     */
    for (uint idx = 0; idx < dataList.size(); ++idx)
    {
        S9sVariantMap  theMap    = dataList[idx].toVariantMap();
        S9sVariantList backups   = theMap["backup"].toVariantList();
        S9sString      root      = theMap["root_dir"].toString();

        for (uint idx2 = 0; idx2 < backups.size(); ++idx2)
        {
            S9sVariantMap  backup    = backups[idx2].toVariantMap();
            S9sVariantList files     = backup["files"].toVariantList();
            S9sBackup      theBackup = theMap;
            int            id        = theBackup.id();
        
            /*
             * Filtering.
             */
            if (options->hasBackupId() && options->backupId() != id)
                continue;

            for (uint idx1 = 0; idx1 < files.size(); ++idx1)
            {
                S9sVariantMap file = files[idx1].toVariantMap();
                S9sString     path = file["path"].toString();
                
                if (options->fullPathRequested())
                {
                    if (!root.endsWith("/"))
                        root += "/";

                    path = root + path;
                }

                if (syntaxHighlight)
                {
                    colorBegin = XTERM_COLOR_RED;
                    colorEnd   = TERM_NORMAL;
                } else {
                    colorBegin = "";
                    colorEnd   = "";
                }

                printf("%s%s%s\n", colorBegin, STR(path), colorEnd);
            }
        }
    }
}

/**
 * This is the one that prints the detailed list of the backups.
 */
void 
S9sRpcReply::printBackupListFilesLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  dataList;
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sFormat       sizeFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       idFormat;
    S9sFormat       parentIdFormat;
    S9sFormat       cidFormat;
    S9sFormat       verifyFormat;
    S9sFormat       incrementalFormat;
    S9sFormat       stateFormat;
    S9sFormat       createdFormat;
    S9sFormat       ownerFormat;
    const char     *colorBegin = "";
    const char     *colorEnd   = "";
   
    // One is RPC 1.0, the other is 2.0.
    if (contains("data"))
        dataList = operator[]("data").toVariantList();
    else if (contains("backup_records"))
        dataList = operator[]("backup_records").toVariantList();
   
    /*
     * Collecting some information.
     */
    for (uint idx = 0; idx < dataList.size(); ++idx)
    {
        S9sVariantMap  theMap     = dataList[idx].toVariantMap();
        S9sBackup      backup     = theMap;
        S9sVariantList backups    = theMap["backup"].toVariantList();
        S9sString      hostName   = backup.backupHost();
        int            clusterId  = backup.clusterId(); 
        S9sString      verifyFlag = backup.verificationFlag();
        S9sString      owner      = backup.configOwner();
        int            id         = backup.id(); 
        int            parentId   = backup.parentId();
        S9sString      status     = backup.status(); 

        #if 0
        S9S_DEBUG("*** hasBackupId: %s", 
                options->hasBackupId() ? "true" : "false");
        S9S_DEBUG("*** bakupId: %d", options->backupId());
        S9S_DEBUG("*** id : %d", id);
        #endif

        if (options->hasBackupId() && options->backupId() != id)
            continue;

        cidFormat.widen(clusterId);
        stateFormat.widen(status);
        hostNameFormat.widen(hostName);
        ownerFormat.widen(owner);
        verifyFormat.widen(verifyFlag);
        incrementalFormat.widen("-");
        
        if (backups.size() == 0u)
        {
            S9sString     sizeString    = "-";
            S9sString     createdString = "-";
            
            createdFormat.widen(createdString);
            sizeFormat.widen(sizeString);
            
            continue;
        }

        for (int backupIdx = 0; backupIdx < backup.nBackups(); ++backupIdx)
        {
            idFormat.widen(id);
            parentIdFormat.widen(parentId);

            for (int fileIdx = 0; 
                    fileIdx < backup.nFiles(backupIdx); ++fileIdx)
            {
                ulonglong size = backup.fileSize(backupIdx, fileIdx).toUll();
                S9sString sizeString;
                S9sString created = backup.fileCreatedString(
                        backupIdx, fileIdx);
        
                sizeString = S9sFormat::toSizeString(size);
                createdFormat.widen(created);
                sizeFormat.widen(sizeString);
            }
        }
    }

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        idFormat.widen("ID");
        parentIdFormat.widen("PI");
        cidFormat.widen("CID");
        verifyFormat.widen("V");
        incrementalFormat.widen("I");
        stateFormat.widen("STATE");
        ownerFormat.widen("OWNER");
        hostNameFormat.widen("HOSTNAME");
        createdFormat.widen("CREATED");
        sizeFormat.widen("SIZE");

        printf("%s", headerColorBegin());
        idFormat.printf("ID");
        parentIdFormat.printf("PI");
        cidFormat.printf("CID");
        verifyFormat.printf("V");
        incrementalFormat.printf("I");
        stateFormat.printf("STATE");
        ownerFormat.printf("OWNER");
        hostNameFormat.printf("HOSTNAME");
        createdFormat.printf("CREATED");
        sizeFormat.printf("SIZE");
        printf("FILENAME");
 
        printf("%s", headerColorEnd());
        printf("\n");
    }
    
    sizeFormat.setRightJustify();
    parentIdFormat.setRightJustify();

    /*
     * Second run, we print things here.
     */
    for (uint idx = 0; idx < dataList.size(); ++idx)
    {
        S9sVariantMap  theMap    = dataList[idx].toVariantMap();
        S9sBackup      backup    = theMap;
        S9sVariantList backups   = theMap["backup"].toVariantList();
        S9sString      hostName  = backup.backupHost();
        int            clusterId = backup.clusterId();
        S9sString      verifyFlag = backup.verificationFlag();
        S9sString      owner     = backup.configOwner();
        int            id        = backup.id();
        int            parentId   = backup.parentId();
        S9sString      status    = backup.status();
        S9sString      root      = backup.rootDir();

        /*
         * Filtering.
         */
        if (options->hasBackupId() && options->backupId() != id)
            continue;

        /*
         *
         */
        if (backups.size() == 0u)
        {
            S9sString     path          = "-";
            S9sString     sizeString    = "-";
            S9sString     createdString = "-";
                
            idFormat.printf(id);
            if (parentId > 0)
                parentIdFormat.printf(parentId);
            else
                parentIdFormat.printf("-");
            
            cidFormat.printf(clusterId);
            verifyFormat.printf(verifyFlag);

            // incremental
            printf("- ");

            printf("%s", backup.statusColorBegin(syntaxHighlight));
            stateFormat.printf(status);
            printf("%s", backup.statusColorEnd(syntaxHighlight));
            
            printf("%s", userColorBegin());
            ownerFormat.printf(owner);
            printf("%s", userColorEnd());

            printf("%s", ipColorBegin());
            hostNameFormat.printf(hostName);
            printf("%s", ipColorEnd());

            createdFormat.printf(createdString);
            sizeFormat.printf(sizeString);
            printf("%s", STR(path));
            printf("\n");

            continue;
        }

        for (int backupIdx = 0; backupIdx < backup.nBackups(); ++backupIdx)
        {
            for (int fileIdx = 0; fileIdx < backup.nFiles(backupIdx); ++fileIdx)
            {
                S9sString   path = backup.fileName(backupIdx, fileIdx);
                ulonglong   size = backup.fileSize(backupIdx, fileIdx).toUll();
                S9sString   sizeString;
                bool        incremental = 
                    backup.incremental(backupIdx, fileIdx).toBoolean();
                S9sString   created = backup.fileCreatedString(
                        backupIdx, fileIdx);

                if (options->fullPathRequested())
                {
                    if (!root.endsWith("/"))
                        root += "/";

                    path = root + path;
                }

                sizeString = S9sFormat::toSizeString(size);

                if (syntaxHighlight)
                {
                    colorBegin = XTERM_COLOR_RED;
                    colorEnd   = TERM_NORMAL;
                } else {
                    colorBegin = "";
                    colorEnd   = "";
                }

                idFormat.printf(id);
        
                if (parentId > 0)
                    parentIdFormat.printf(parentId);
                else
                    parentIdFormat.printf("-");

                cidFormat.printf(clusterId);
                verifyFormat.printf(verifyFlag);
        
                if (incremental)
                    printf("I ");
                else
                    printf("F ");
                
                printf("%s", backup.statusColorBegin(syntaxHighlight));
                stateFormat.printf(status);
                printf("%s", backup.statusColorEnd(syntaxHighlight));

                printf("%s", userColorBegin());
                ownerFormat.printf(owner);
                printf("%s", userColorEnd());

                printf("%s", ipColorBegin());
                hostNameFormat.printf(hostName);
                printf("%s", ipColorEnd());

                createdFormat.printf(created);
                sizeFormat.printf(sizeString);
                printf("%s%s%s", colorBegin, STR(path), colorEnd);
                printf("\n");
            }
        }
    }

    /*
     * Footer.
     */
    if (!options->isBatchRequested() && contains("total"))
    {
        int total = operator[]("total").toInt();

        printf("Total %d\n", total);
    }
}

void 
S9sRpcReply::printMaintenanceListBrief()
{
    S9sVariantList  recordList;
    
    recordList = operator[]("maintenance_records").toVariantList();
    for (uint idx = 0; idx < recordList.size(); ++idx)
    {
        S9sVariantMap  record  = recordList[idx].toVariantMap();
        S9sVariantList periods = record["maintenance_periods"].toVariantList();

        for (uint idx1 = 0; idx1 < periods.size(); ++idx1)
        {
            S9sVariantMap period = periods[idx1].toVariantMap();
            S9sString     uuid      = period["UUID"].toString();

            printf("%s\n", STR(uuid));
        }
    }
}

/**
 * Prints the maintenance list in the long format, one maintenance period in
 * every line.
 */
void 
S9sRpcReply::printMaintenanceListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  recordList;
    S9sFormat       ownerFormat;
    S9sFormat       uuidFormat;
    S9sFormat       groupOwnerFormat;
    S9sFormat       startFormat;
    S9sFormat       endFormat;
    S9sFormat       nameFormat;

    recordList = operator[]("maintenance_records").toVariantList();

    /*
     * First run, we collect some information.
     */
    for (uint idx = 0; idx < recordList.size(); ++idx)
    {
        S9sVariantMap  record  = recordList[idx].toVariantMap();
        S9sVariantList periods = record["maintenance_periods"].toVariantList();
        bool           isClusterMaintenance = record.contains("cluster_id");
        S9sString      hostName;
        S9sString      clusterName;
        S9sVariantMap  clusterMap;
        
        if (!isClusterMaintenance)
        {
            hostName = record["hostname"].toString();
            nameFormat.widen(hostName);
        } else if (isClusterMaintenance)
        {
            clusterMap  = record["cluster"].toVariantMap();
            clusterName = clusterMap["cluster_name"].toString();
            nameFormat.widen(clusterName);
        }
       
        for (uint idx1 = 0; idx1 < periods.size(); ++idx1)
        {
            S9sVariantMap period    = periods[idx1].toVariantMap();
            S9sString     userName  = period["username"].toString();
            S9sString     groupName = period["groupname"].toString();
            S9sString     uuid      = period["UUID"].toString();
            S9sDateTime   start, end;
            S9sString     startString, endString;
            
            if (!options->fullUuid())
                uuid = uuid.substr(0, 7);

            start.parse(period["initiate"].toString());
            end.parse(period["deadline"].toString());
            startString = options->formatDateTime(start);
            endString   = options->formatDateTime(end);
            
            uuidFormat.widen(uuid);
            ownerFormat.widen(userName);
            groupOwnerFormat.widen(groupName);
            startFormat.widen(startString);
            endFormat.widen(endString);
        }
    }
    
    if (!options->isNoHeaderRequested())
    {
        uuidFormat.widen("UUID");
        ownerFormat.widen("OWNER");
        groupOwnerFormat.widen("GROUP");
        startFormat.widen("START");
        endFormat.widen("END");
        nameFormat.widen("HOST/CLUSTER");

        printf("%s", headerColorBegin());
        printf("ST ");
        uuidFormat.printf("UUID");
        ownerFormat.printf("OWNER");
        groupOwnerFormat.printf("GROUP");
        startFormat.printf("START");
        endFormat.printf("END");
        nameFormat.printf("HOST/CLUSTER");
        printf("REASON");
        
        printf("%s", headerColorEnd());

        printf("\n");
    }

    /*
     * Second run, we print.
     */
    for (uint idx = 0; idx < recordList.size(); ++idx)
    {
        S9sVariantMap  record   = recordList[idx].toVariantMap();
        S9sVariantList periods = record["maintenance_periods"].toVariantList();
        bool           isHostMaintenance = record.contains("hostname");
        bool           isClusterMaintenance = record.contains("cluster_id");
        S9sString      hostName;
        S9sString      clusterName;
        S9sVariantMap  clusterMap;

        if (isHostMaintenance)
        {
            hostName = record["hostname"].toString();
        } else if (isClusterMaintenance)
        {
            clusterMap  = record["cluster"].toVariantMap();
            clusterName = clusterMap["cluster_name"].toString();
        }

        for (uint idx1 = 0; idx1 < periods.size(); ++idx1)
        {
            S9sVariantMap period = periods[idx1].toVariantMap();
            S9sString     userName  = period["username"].toString();
            S9sString     groupName = period["groupname"].toString();
            S9sString     reason    = period["reason"].toString();
            S9sString     uuid      = period["UUID"].toString();
            S9sDateTime   start, end;
            S9sString     startString, endString;
            bool          isActive = period["is_active"].toBoolean();

            start.parse(period["initiate"].toString());
            end.parse(period["deadline"].toString());
            startString = options->formatDateTime(start);
            endString   = options->formatDateTime(end);

            if (!options->fullUuid())
                uuid = uuid.substr(0, 7);

            printf("%c", isActive ? 'A' : '-');
            if (isHostMaintenance)
                printf("h ");
            else if (isClusterMaintenance)
                printf("c ");
            else 
                printf("- ");
                
            printf("%s ", STR(uuid));

            printf("%s", userColorBegin());
            ownerFormat.printf(userName);
            printf("%s", userColorEnd());
            
            printf("%s", groupColorBegin(groupName));
            groupOwnerFormat.printf(groupName);
            printf("%s", groupColorEnd());

            startFormat.printf(startString);
            endFormat.printf(endString);

            if (isHostMaintenance)
            {
                nameFormat.printf(STR(hostName));
            } else {
                printf("%s", clusterColorBegin());
                nameFormat.printf(STR(clusterName));
                printf("%s", groupColorEnd());
            }

            printf("%s ", STR(reason));
            printf("\n");
        }
    }

    if (!options->isBatchRequested())
        printf("Total: %d\n", operator[]("total").toInt());
}

void 
S9sRpcReply::printGroupListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  groupList = operator[]("groups").toVariantList();
    
    for (uint idx = 0; idx < groupList.size(); ++idx)
    {
        S9sVariantMap  groupMap      = groupList[idx].toVariantMap();
        S9sGroup       group         = groupMap;
        const char    *groupColorBegin = "";
        const char    *groupColorEnd   = "";

        if (!options->isStringMatchExtraArguments(group.groupName()))
            continue;

        if (syntaxHighlight)
        {
            groupColorBegin = XTERM_COLOR_CYAN;
            groupColorEnd   = TERM_NORMAL;
        }
        
        printf("%s%s%s\n", 
                groupColorBegin, 
                STR(group.groupName()), 
                groupColorEnd);
    }
}

void 
S9sRpcReply::printGroupListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  groupList = operator[]("groups").toVariantList();
    
    for (uint idx = 0; idx < groupList.size(); ++idx)
    {
        S9sVariantMap  groupMap      = groupList[idx].toVariantMap();
        S9sString      groupName     = groupMap["group_name"].toString();
        const char    *groupColorBegin = "";
        const char    *groupColorEnd   = "";

        if (syntaxHighlight)
        {
            groupColorBegin = XTERM_COLOR_CYAN;
            groupColorEnd   = TERM_NORMAL;
        }
        
        printf("%s%s%s\n", groupColorBegin, STR(groupName), groupColorEnd);
    }
}

/**
 * \param userName The name of the user to return.
 * \returns The user object from the reply.
 */
S9sUser
S9sRpcReply::getUser(
        const S9sString &userName)
{
    if (contains("users"))
    {
        S9sVariantList  userList = operator[]("users").toVariantList();
        
        for (uint idx = 0; idx < userList.size(); ++idx)
        {
            S9sVariantMap  userMap      = userList[idx].toVariantMap();
            S9sUser        user         = userMap;
        
            if (user.userName() == userName)
                return user;
        }
    }

    return S9sUser();
}

/**
 * Prints the user list in either short or long format.
 */
void 
S9sRpcReply::printUserList()
{
    S9sOptions *options = S9sOptions::instance();
    
    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
        return;
    }

    if (options->isStatRequested())
        printUserListStat();
    else if (options->isLongRequested())
        printUserListLong();
    else
        printUserListBrief();
}

void 
S9sRpcReply::printUserListStat()
{
    S9sVariantList  userList    = operator[]("users").toVariantList();
    S9sOptions     *options     = S9sOptions::instance();
    S9sString       groupFilter = options->group();
    bool            whoAmIRequested = options->isWhoAmIRequested();
    int             authUserId  = operator[]("request_user_id").toInt();

    for (uint idx = 0; idx < userList.size(); ++idx)
    {
        S9sVariantMap  userMap      = userList[idx].toVariantMap();
        S9sUser        user         = userMap;
        S9sString      userName     = user.userName();
        int            userId       = user.userId();
        
        //
        // Filtering.
        //
        if (whoAmIRequested && userId != authUserId)
            continue;

        if (!options->isStringMatchExtraArguments(userName))
            continue;
        
        if (!groupFilter.empty() && !user.isMemberOf(groupFilter))
            continue;

        printUserStat(user);
    }
}

void 
S9sRpcReply::printUserStat(
        S9sUser &user)
{
    S9sOptions *options = S9sOptions::instance();
    int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    S9sString   title;
    
    //
    // The title that is in inverse. 
    //
#if 1
    if (!user.fullName().empty())
        title.sprintf("%s", STR(user.fullName()));
    else
#endif
        title.sprintf("%s", STR(user.userName()));

    printf("%s", TERM_INVERSE);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL);
   
    printObjectStat(user);
    
    //
    // "    Name: galera_001                          Owner: pipas/users"
    //
#if 0
    printf("%s    Name:%s ", greyBegin, greyEnd);
    printf("%s", userColorBegin());
    printf("%-25s ", STR(user.userName()));
    printf("%s", userColorEnd());
    
    printf("%s   Owner:%s ", greyBegin, greyEnd);
    printf("%s%s%s/%s%s%s ", 
            userColorBegin(), STR(user.ownerName()), userColorEnd(),
            groupColorBegin(user.groupOwnerName()), 
            STR(user.groupOwnerName()), 
            groupColorEnd());
    
    printf("\n");

    //
    // "      ID: 3                         Disabled: no"
    //
    printf("%s      ID:%s ", greyBegin, greyEnd);
    printf("%-5d ", user.userId());

    printf("                    %sDisabled:%s ", greyBegin, greyEnd);
    printf("%s", user.isDisabled() ? "yes" : "no");
    printf("\n");
#endif
    
    //
    // "Fullname: László Pere                  Email: laszlo@severalnines.com"
    //
    printf("%sFullname:%s ", greyBegin, greyEnd);
    printf("%-28s ", STR(user.fullName("-")));
    printf("\n");
   
    //
    //
    //
    printf("%s   Email:%s ", greyBegin, greyEnd);
    printf("%s ", STR(user.emailAddress("-")));
    printf("\n");
    
    //
    //
    //
    printf("%sDisabled:%s ", greyBegin, greyEnd);
    printf("%s", user.isDisabled() ? "yes" : "no");
    printf("\n");

    
    //
    // " Suspend: no                   Failed logins: 0"
    //
    printf("%s Suspend:%s ", greyBegin, greyEnd);
    printf("%-19s ", user.isSuspended() ? "yes" : "no");
    
    printf("%s         Failed logins:%s ", greyBegin, greyEnd);
    printf("%d", user.nFailedLogins());
    printf("\n");
    
    //
    // "  Groups: users"
    //
    printf("%s  Groups:%s ", greyBegin, greyEnd);
    printf("%-30s ", STR(user.groupNames(", ")));
    printf("\n");

    //
    // " Created: 2017-10-26T12:08:55.945Z"
    //
    printf("%s Created:%s ", greyBegin, greyEnd);
    printf("%-30s ", STR(user.createdString("-")));
    printf("\n");

    //
    // "   Login: 2017-10-26T12:10:37.762Z"
    //
    printf("%s   Login:%s ", greyBegin, greyEnd);
    printf("%-30s ", STR(user.lastLoginString("-")));
    printf("\n");
    
    //
    // 
    //
    printf("%s Failure:%s ", greyBegin, greyEnd);
    printf("%-24s ", STR(user.failedLoginString("-")));
    
    
    printf("\n\n");
}

/**
 * Prints the user list in short format.
 */
void 
S9sRpcReply::printUserListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sString       formatString = options->longBackupFormat();
    S9sVariantList  userList = operator[]("users").toVariantList();
    int             authUserId = operator[]("request_user_id").toInt();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       groupFilter     = options->group();
    bool            whoAmIRequested = options->isWhoAmIRequested();
    const char     *colorBegin = "";
    const char     *colorEnd   = "";

    if (options->hasUserFormat())
        formatString = options->userFormat();

    if (!formatString.empty())
    {
        for (uint idx = 0; idx < userList.size(); ++idx)
        {
            S9sVariantMap  userMap      = userList[idx].toVariantMap();
            S9sUser        user         = userMap;
            int            userId       = user.userId();
            S9sString      userName     = user.userName();

            /*
             * Filtering.
             */
            if (whoAmIRequested && userId != authUserId)
                continue;
        
            if (!options->isStringMatchExtraArguments(userName))
                continue;
            
            if (!groupFilter.empty() && !user.isMemberOf(groupFilter))
                continue;
   
            printf("%s", STR(user.toString(syntaxHighlight, formatString)));
            
        }

        return;
    }

    /*
     * Printing the user names.
     */
    for (uint idx = 0; idx < userList.size(); ++idx)
    {
        S9sVariantMap  userMap      = userList[idx].toVariantMap();
        S9sUser        user         = userMap;
        S9sString      userName     = user.userName();
        int            userId       = user.userId();
        
        //
        // Filtering.
        //
        if (whoAmIRequested && userId != authUserId)
            continue;

        if (!options->isStringMatchExtraArguments(userName))
            continue;
        
        if (!groupFilter.empty() && !user.isMemberOf(groupFilter))
            continue;

        /*
         * Syntax highlight.
         */
        if (syntaxHighlight)
        {
            colorBegin = XTERM_COLOR_ORANGE;
            colorEnd   = TERM_NORMAL;
        } else {
            colorBegin = "";
            colorEnd   = "";
        }

        printf("%s%s%s\n", colorBegin, STR(userName), colorEnd);
    }
}

/**
 * Prints the user list in long format.
 */
void 
S9sRpcReply::printUserListLong()
{
    S9sOptions     *options  = S9sOptions::instance();
    S9sString       formatString = options->longBackupFormat();
    S9sVariantList  userList = operator[]("users").toVariantList();
    int             authUserId = operator[]("request_user_id").toInt();
    bool            whoAmIRequested = options->isWhoAmIRequested();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       groupFilter     = options->group();
    const char     *colorBegin = "";
    const char     *colorEnd   = "";
    const char     *groupColorBegin = "";
    const char     *groupColorEnd   = "";
    S9sFormat       idFormat;
    S9sFormat       userNameFormat;
    S9sFormat       groupNamesFormat;
    S9sFormat       emailFormat;

    if (options->hasUserFormat())
        formatString = options->userFormat();

    if (!formatString.empty())
    {
        for (uint idx = 0; idx < userList.size(); ++idx)
        {
            S9sVariantMap  userMap      = userList[idx].toVariantMap();
            S9sUser        user         = userMap;
            int            userId       = user.userId();
            S9sString      userName     = user.userName();

            /*
             * Filtering.
             */
            if (whoAmIRequested && userId != authUserId)
                continue;
        
            if (!options->isStringMatchExtraArguments(userName))
                continue;
        
            if (!groupFilter.empty() && !user.isMemberOf(groupFilter))
                continue;
   
            printf("%s", STR(user.toString(syntaxHighlight, formatString)));
        }

        if (!options->isBatchRequested())
            printf("Total: %d\n", operator[]("total").toInt());

        return;
    }

    /*
     * Going through first and collecting some informations.
     */
    for (uint idx = 0; idx < userList.size(); ++idx)
    {
        S9sVariantMap  userMap      = userList[idx].toVariantMap();
        S9sUser        user         = userMap;
        S9sString      userName     = user.userName();
        S9sString      emailAddress = user.emailAddress();
        int            userId       = user.userId();
        S9sString      groupNames   = user.groupNames(); 
        
        /*
         * Filtering.
         */
        if (whoAmIRequested && userId != authUserId)
            continue;
        
        if (!options->isStringMatchExtraArguments(userName))
            continue;

        if (!groupFilter.empty() && !user.isMemberOf(groupFilter))
            continue;

        if (groupNames.empty())
            groupNames = "-";
        
        if (emailAddress.empty())
            emailAddress = "-";

        userNameFormat.widen(userName);
        emailFormat.widen(emailAddress);
        idFormat.widen(userId);
        groupNamesFormat.widen(groupNames);
    }

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        idFormat.widen("ID");
        userNameFormat.widen("UNAME");
        groupNamesFormat.widen("GNAME");
        emailFormat.widen("EMAIL");

        printf("%s", headerColorBegin());
        printf("A ");
        idFormat.printf("ID");
        userNameFormat.printf("UNAME");
        groupNamesFormat.printf("GNAME");
        emailFormat.printf("EMAIL");
        printf("REALNAME");
        printf("%s", headerColorEnd());

        printf("\n");
    }

    /*
     * Going through again and printing.
     */
    for (uint idx = 0; idx < userList.size(); ++idx)
    {
        S9sVariantMap  userMap      = userList[idx].toVariantMap();
        S9sUser        user         = userMap;
        S9sString      userName     = user.userName();
        int            userId       = user.userId();
        S9sString      emailAddress = user.emailAddress();
        S9sString      fullName     = user.fullName();
        S9sString      groupNames   = user.groupNames(); 
       
        //
        // Filtering.
        //
        if (whoAmIRequested && userId != authUserId)
            continue;
        
        if (!options->isStringMatchExtraArguments(userName))
            continue;
        
        if (!groupFilter.empty() && !user.isMemberOf(groupFilter))
            continue;
       
        if (groupNames.empty())
            groupNames = "-";
        
        if (emailAddress.empty())
            emailAddress = "-";

        if (fullName.empty())
            fullName = "-";

        if (syntaxHighlight)
        {
            colorBegin      = XTERM_COLOR_ORANGE;
            colorEnd        = TERM_NORMAL;
            groupColorBegin = XTERM_COLOR_CYAN;
            groupColorEnd   = TERM_NORMAL;
        }

        //
        // Printing the fields.
        //
        if (userId == authUserId)
            printf("A ");
        else
            printf("- ");
        
        idFormat.printf(userId);

        printf("%s", colorBegin);
        userNameFormat.printf(userName);
        printf("%s", colorEnd);
        
        printf("%s", groupColorBegin);
        groupNamesFormat.printf(groupNames);
        printf("%s", groupColorEnd);

        emailFormat.printf(emailAddress);

        printf("%s", STR(fullName));
        printf("\n");
    }
    
    if (!options->isBatchRequested())
        printf("Total: %d\n", operator[]("total").toInt());
}

void
S9sRpcReply::printCpuStatLine1()
{
    S9sOptions      *options = S9sOptions::instance();
    bool             syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList   theList = operator[]("data").toVariantList();
    S9sVariantMap    listMap;
    const char      *numberStart = "";
    const char      *numberEnd   = "";
    S9sVariantMap    keys;
    S9sMap<int, int> hostIds;
    double           user  = 0.0;
    double           sys   = 0.0;
    double           idle  = 0.0;
    double           wait  = 0.0;
    double           steal = 0.0;


    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap  = theList[idx].toVariantMap();
        S9sString     key     = theMap["samplekey"].toString();
        ulonglong     created = theMap["created"].toULongLong();

        if (!listMap.contains(key))
        {
            listMap[key] = theMap;
            continue;
        }

        if (listMap[key]["created"].toULongLong() < created)
            listMap[key] = theMap;
    }

    if (syntaxHighlight)
    {
        numberStart = TERM_BOLD;
        numberEnd   = TERM_NORMAL;
    }


    foreach (const S9sVariant variant, listMap)
    {
        S9sVariantMap theMap    = variant.toVariantMap();
        int           hostId    = theMap["hostid"].toInt();
        double        thisUser  = theMap["user"].toDouble();
        double        thisSys   = theMap["sys"].toDouble();
        double        thisIdle  = theMap["idle"].toDouble();
        double        thisWait  = theMap["iowait"].toDouble();
        double        thisSteal = theMap["steal"].toDouble();
        
        //printf("-> \n%s\n", STR(theMap.toString()));
        user  += thisUser;
        sys   += thisSys;
        idle  += thisIdle;
        wait  += thisWait;
        steal += thisSteal;

        hostIds[hostId] = true;
    }
    
    user  /= listMap.size();
    sys   /= listMap.size();
    idle  /= listMap.size();
    wait  /= listMap.size();
    steal /= listMap.size();

    user  *= 100.0;
    sys   *= 100.0;
    idle  *= 100.0;
    wait  *= 100.0;
    steal *= 100.0;
    
    printf("%s%d%s hosts, ", numberStart, (int)hostIds.size(), numberEnd);
    printf("%s%d%s cores,", numberStart, (int)listMap.size(), numberEnd);
    printf("%s%5.1f%s us,",  numberStart, user, numberEnd);
    printf("%s%5.1f%s sy,", numberStart, sys, numberEnd);
    printf("%s%5.1f%s id,",  numberStart, idle, numberEnd);
    printf("%s%5.1f%s wa,", numberStart, wait, numberEnd);
    printf("%s%5.1f%s st,", numberStart, steal, numberEnd);
}

/**
 *
    {
        "class_name": "CmonMemoryStats",
        "created": 1475746585,
        "hostid": 3,
        "interval": 35612,
        "memoryutilization": 0.21689,
        "pgpgin": 0,
        "pgpgout": 208,
        "pswpin": 0,
        "pswpout": 0,
        "rambuffers": 35971072,
        "ramcached": 1982054400,
        "ramfree": 1229369344,
        "ramfreemin": 1229369344,
        "ramtotal": 4146794496,
        "sampleends": 1475746585,
        "samplekey": "CmonMemoryStats-3",
        "swapfree": 0,
        "swaptotal": 0,
        "swaputilization": 0
    }, 
 *
 * total,    used, free,    buffers,    cached
 * ramtotal,       ramfree, rambuffers, ramcached
 */
void
S9sRpcReply::printMemoryStatLine1()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList    = operator[]("data").toVariantList();
    double          sumTotal   = 0.0;
    double          sumFree    = 0.0;
    double          sumBuffers = 0.0;
    double          sumCached  = 0.0;
    S9sVariantMap   listMap;
    const char     *numberStart = "";
    const char     *numberEnd   = "";
    
    if (syntaxHighlight)
    {
        numberStart = TERM_BOLD;
        numberEnd   = TERM_NORMAL;
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap  = theList[idx].toVariantMap();
        S9sString     key     = theMap["samplekey"].toString();
        ulonglong     created = theMap["created"].toULongLong();

        if (!listMap.contains(key))
        {
            listMap[key] = theMap;
            continue;
        }

        if (listMap[key]["created"].toULongLong() < created)
            listMap[key] = theMap;
    }
    
    foreach (const S9sVariant variant, listMap)
    {
        S9sVariantMap theMap    = variant.toVariantMap();
        ulonglong     total     = theMap["ramtotal"].toULongLong();
        ulonglong     free      = theMap["ramfree"].toULongLong();
        ulonglong     buffers   = theMap["rambuffers"].toULongLong();
        ulonglong     cached    = theMap["ramcached"].toULongLong();

        sumTotal   += total;
        sumFree    += free;
        sumBuffers += buffers;
        sumCached  += cached;
    }

    sumTotal   /= 1024 * 1024 * 1024.0;
    sumFree    /= 1024 * 1024 * 1024.0;
    sumBuffers /= 1024 * 1024 * 1024.0;
    sumCached  /= 1024 * 1024 * 1024.0;

    printf("GiB Mem : ");
    printf("%s%.1f%s total, ",   numberStart, sumTotal, numberEnd);
    printf("%s%.1f%s free, ",    numberStart, sumFree, numberEnd);
    printf("%s%.1f%s used, ",    numberStart, sumTotal - (sumFree + sumBuffers + sumCached), numberEnd);
    printf("%s%.1f%s buffers, ", numberStart, sumBuffers, numberEnd);
    printf("%s%.1f%s cached",    numberStart, sumCached, numberEnd);
}

void
S9sRpcReply::printMemoryStatLine2()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList    = operator[]("data").toVariantList();
    ulonglong       sumTotal   = 0ull;
    ulonglong       sumFree    = 0ull;
    S9sVariantMap   listMap;
    const char     *numberStart = "";
    const char     *numberEnd   = "";
        
    if (syntaxHighlight)
    {
        numberStart = TERM_BOLD;
        numberEnd   = TERM_NORMAL;
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap  = theList[idx].toVariantMap();
        S9sString     key     = theMap["samplekey"].toString();
        ulonglong     created = theMap["created"].toULongLong();

        if (!listMap.contains(key))
        {
            listMap[key] = theMap;
            continue;
        }

        if (listMap[key]["created"].toULongLong() < created)
            listMap[key] = theMap;
    }
    
    foreach (const S9sVariant variant, listMap)
    {
        S9sVariantMap theMap    = variant.toVariantMap();
        ulonglong     total     = theMap["swaptotal"].toULongLong();
        ulonglong     free      = theMap["swapfree"].toULongLong();

        sumTotal   += total;
        sumFree    += free;
    }

    sumTotal   /= 1024 * 1024 * 1024;
    sumFree    /= 1024 * 1024 * 1024;

    printf("GiB Swap: ");
    printf("%s%llu%s total, ", numberStart, sumTotal, numberEnd);
    printf("%s%llu%s used, ",  numberStart, sumTotal - sumFree, numberEnd);
    printf("%s%llu%s free, ",  numberStart, sumFree, numberEnd);
}

/**
 * \param percent the percent (between 0.0 and 100.0) that is shown by the
 *   progress bar.
 * \param syntaxHighlight true to use ANSI terminal color sequences.
 * \returns the string that looks like a progress bar.
 *
 * Creates a string that shows up a progress bar using UTF-8 characters.
 */
S9sString 
S9sRpcReply::progressBar(
        double percent,
        bool   syntaxHighlight)
{
    S9sOptions *options = S9sOptions::instance();
    bool        ascii = options->onlyAscii();
    S9sString   retval;
    int         nBars;
    int         remain;

    if (percent < 0.0)
        percent = 0.0;
    else if (percent > 100.0)
        percent = 100.0;

    nBars   = percent / 10;
    remain  = (int) percent % 10;

    retval += "[";

    if (syntaxHighlight)
        retval += XTERM_COLOR_BLUE;

    for (int n = 1; n <= nBars; ++n)
        retval += ascii ? "#" : "█";

    if (percent < 100.0)
    {
        switch (remain)
        {
            case 0:
                retval += " ";
                break;

        	case 1:
                retval += ascii ? " " : "▏";
                break;

        	case 2:
                retval += ascii ? " " : "▎";
                break;

        	case 3:
                retval += ascii ? " " : "▍";
                break;

        	case 4:
                retval += ascii ? " " : "▌";
                break;

        	case 5:
                retval += ascii ? " " : "▋";
                break;

        	case 6:
        	case 7:
                retval += ascii ? " " : "▊";
                break;

        	case 8:
        	case 9:
                retval += ascii ? " " : "▉";
                break;
        }
    }

    if (syntaxHighlight)
        retval += TERM_NORMAL;

    for (int n = nBars; n < 9; ++n)
        retval += " ";

    retval += "] ";

    return retval;
}

S9sString 
S9sRpcReply::progressBar(
        bool   syntaxHighlight)
{
    S9sOptions *options = S9sOptions::instance();
    bool        ascii = options->onlyAscii();
    S9sString   retval;
    int         timeCycle = time(NULL) % 20;
    int         position;

    if (timeCycle < 10)
        position = timeCycle;
    else
        position = 19 - timeCycle;

    //retval.sprintf("%3d %3d", timeCycle, position);

    retval += "[";

    // Left spaces.
    for (int n = 0; n < position; ++n)
        retval += " ";

    // The one bar.
    if (syntaxHighlight)
        retval += XTERM_COLOR_BLUE;

    retval += ascii ? "#" : "█";

    if (syntaxHighlight)
        retval += TERM_NORMAL;

    for (int n = position + 1; n < 10; ++n)
        retval += " ";

    retval += "] ";

    return retval;
}
/**
 * \returns The map about the given cluster or an empty map if the reply has no
 *   cluster description in it.
 */
S9sVariantMap
S9sRpcReply::clusterMap(
        const int clusterId)
{
    S9sVariantList  theList = clusters();
    S9sVariantMap   retval;

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap      = theList[idx].toVariantMap();

        if (theMap["cluster_id"].toInt() != clusterId)
            continue;

        retval = theMap;
        break;
    }

    return retval;
}

bool
S9sRpcReply::useSyntaxHighLight()
{
    S9sOptions *options = S9sOptions::instance();
   
    return options->useSyntaxHighlight();
}

const char *
S9sRpcReply::optNameColorBegin() const
{
    if (useSyntaxHighLight())
        return TERM_BLUE;

    return "";
}

const char *
S9sRpcReply::optNameColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::numberColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_NUMBER;

    return "";
}

const char *
S9sRpcReply::numberColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::clusterColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_BLUE;

    return "";
}

const char *
S9sRpcReply::clusterColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::databaseColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_DATABASE;

    return "";
}

const char *
S9sRpcReply::databaseColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::userColorBegin() 
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_ORANGE;

    return "";
}

const char *
S9sRpcReply::userColorEnd() 
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::executableColorBegin(
        const S9sString &executable) 
{
    if (useSyntaxHighLight())
    {
        if (executable.contains("mysql") || 
                executable.contains("cmon") || 
                executable == "postgres")
        {
            return "\033[38;5;46m";
        }

        return "\033[38;5;34m";
    }

    return "";
}

const char *
S9sRpcReply::executableColorEnd() 
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::serverColorBegin() 
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_SERVER;

    return "";
}

const char *
S9sRpcReply::serverColorEnd() 
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::groupColorBegin(
        const S9sString &groupName) 
{
    if (useSyntaxHighLight())
    {
        if (groupName == "0")
            return XTERM_COLOR_RED;
        else
            return XTERM_COLOR_CYAN;
    }

    return "";
}

const char *
S9sRpcReply::groupColorEnd() 
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::headerColorBegin() const
{
    if (useSyntaxHighLight())
        return TERM_BOLD;
        //return TERM_INVERSE;

    return "";
}

const char *
S9sRpcReply::headerColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::folderColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_YELLOW;

    return "";
}

const char *
S9sRpcReply::folderColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::containerColorBegin(
        int stateAsChar)
{
    if (useSyntaxHighLight())
    {
        if (stateAsChar == 't')
            return XTERM_COLOR_RED;
        else if (stateAsChar == 's')
            return XTERM_COLOR_RED;
        else if (stateAsChar == '?')
            return XTERM_COLOR_RED;
        else if (stateAsChar == 'q')
            return XTERM_COLOR_YELLOW;

        return XTERM_COLOR_NODE;
    }

    return "";
}

const char *
S9sRpcReply::containerColorEnd() 
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::ipColorBegin(
        const S9sString &ip)
{
    if (useSyntaxHighLight() && ip.looksLikeIpAddress())
        return XTERM_COLOR_IP;

    return "";
}

const char *
S9sRpcReply::ipColorEnd(
        const S9sString &ip) 
{
    if (useSyntaxHighLight() && ip.looksLikeIpAddress())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::greyColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_DARK_GRAY;

    return "";
}

const char *
S9sRpcReply::greyColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::typeColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_GREEN;

    return "";
}

const char *
S9sRpcReply::typeColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::propertyColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_LIGHT_GREEN;

    return "";
}

const char *
S9sRpcReply::propertyColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::hostStateColorBegin(
        const S9sString status)
{
    if (useSyntaxHighLight())
    {
        if (status == "CmonHostRecovery" || status == "CmonHostShutDown")
        {
            return XTERM_COLOR_YELLOW;
        } else if (status == "CmonHostUnknown" || status == "CmonHostOffLine")
        {
            return XTERM_COLOR_RED;
        } else {
            return XTERM_COLOR_GREEN;
        }
    }

    return "";
}

const char *
S9sRpcReply::hostStateColorEnd()
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}


/**
 * This method returns the ANSI color sequence for the beginning of a cluster
 * state string. The color of course depends on the state itself.
 */
const char *
S9sRpcReply::clusterStateColorBegin(
        S9sString state)
{
    if (useSyntaxHighLight())
    {
        if (state == "DEGRADED")
            return XTERM_COLOR_YELLOW;
        else if (state == "FAILURE" || state == "UNKNOWN")
            return XTERM_COLOR_RED;
        else if (state == "STARTED")
            return XTERM_COLOR_GREEN;
        else if (state == "STOPPED")
            return XTERM_COLOR_YELLOW;
        else if (state == "SHUTTING_DOWN")
            return XTERM_COLOR_YELLOW;
        else if (state == "RUNNING")
            // This is actually for containers...
            return XTERM_COLOR_GREEN;
        else
            return TERM_NORMAL;
    }

    return "";
}

const char *
S9sRpcReply::clusterStateColorEnd()
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sRpcReply::fileColorBegin(
        const S9sString &fileName) 
{
    if (useSyntaxHighLight())
    {
        if (fileName.endsWith(".gz"))
            return XTERM_COLOR_RED;
        else if (fileName.endsWith(".tar"))
            return XTERM_COLOR_ORANGE;
        else if (fileName.endsWith(".log"))
            return XTERM_COLOR_PURPLE;
        else if (fileName.endsWith(".cnf"))
            return XTERM_COLOR_LIGHT_PURPLE;
        else if (fileName.endsWith(".conf"))
            return XTERM_COLOR_LIGHT_PURPLE;
        else if (fileName.endsWith("/config"))
            return XTERM_COLOR_LIGHT_PURPLE;
        else if (fileName.endsWith(".ini"))
            return XTERM_COLOR_LIGHT_PURPLE;
        else if (fileName.endsWith(".pid"))
            return XTERM_COLOR_LIGHT_RED;
        else
            return XTERM_COLOR_11;
    }

    return "";
}

const char *
S9sRpcReply::fileColorEnd() 
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

void 
S9sRpcReply::printMetaTypeList()
{
    S9sOptions *options = S9sOptions::instance();
    
    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (options->isLongRequested())
        printMetaTypeListLong();
    else
        printMetaTypeListBrief();
}


void 
S9sRpcReply::printMetaTypeListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("metatype_info").toVariantList();
    S9sFormat       nameFormat;

    /*
     * First run-through: collecting some information.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap typeMap      = theList[idx].toVariantMap();
        S9sString     typeName     = typeMap["type_name"].toString();
        S9sString     description  = typeMap["description"].toString();
       
        if (typeName.contains("*"))
            continue;

        if (!options->isStringMatchExtraArguments(typeName))
            continue;

        nameFormat.widen(typeName);
    }
    
    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        nameFormat.widen("NAME");
            
        printf("%s", headerColorBegin());
         
        nameFormat.printf("NAME");
        printf("DESCRIPTION");

        printf("%s", headerColorEnd());
        printf("\n");
    }

    /*
     * Printing the actual list.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap typeMap      = theList[idx].toVariantMap();
        S9sString     typeName     = typeMap["type_name"].toString();
        S9sString     description  = typeMap["description"].toString();

        if (typeName.contains("*"))
            continue;

        if (!options->isStringMatchExtraArguments(typeName))
            continue;

        if (description.empty())
            description = "-";

        printf("%s", typeColorBegin());
        nameFormat.printf(typeName);
        printf("%s", typeColorEnd());

        printf("%s", STR(description));
        printf("\n");
    }
}

/**
 * Lists the available types in brief format.
 */
void 
S9sRpcReply::printMetaTypeListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("metatype_info").toVariantList();
    int             isTerminal    = options->isTerminal();
    int             terminalWidth = options->terminalWidth();
    S9sFormat       nameFormat;
    int             currentPosition = 0;
    
    /*
     * First run-through: collecting some information.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap typeMap      = theList[idx].toVariantMap();
        S9sString     typeName     = typeMap["type_name"].toString();
        S9sString     description  = typeMap["description"].toString();
        
        // There is a bug in the metatype system.
        if (typeName.contains(" "))
            continue;

        if (!options->isStringMatchExtraArguments(typeName))
            continue;

        nameFormat.widen(typeName);
    }
    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap typeMap      = theList[idx].toVariantMap();
        S9sString     typeName     = typeMap["type_name"].toString();
        
        // There is a bug in the metatype system.
        if (typeName.contains(" "))
            continue;

        if (!options->isStringMatchExtraArguments(typeName))
            continue;

        printf("%s", typeColorBegin());
        nameFormat.printf(typeName);
        printf("%s", typeColorEnd());

        currentPosition += nameFormat.realWidth();
        if (currentPosition + nameFormat.realWidth() > terminalWidth ||
                !isTerminal)
        {
            printf("\n");
            currentPosition = 0;
        }
    }

    if (currentPosition > 0)
        printf("\n");
}

void 
S9sRpcReply::printMetaTypePropertyList()
{
    S9sOptions *options = S9sOptions::instance();
    
    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
    else if (options->isLongRequested())
        printMetaTypePropertyListLong();
    else
        printMetaTypePropertyListBrief();
}

/**
 * This function will print the properties of a type from the metatype system in
 * a detailed long list.
 */
void 
S9sRpcReply::printMetaTypePropertyListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    int             isTerminal    = options->isTerminal();
    int             terminalWidth = options->terminalWidth();
    S9sVariantList  theList = operator[]("metatype_info").toVariantList();
    S9sFormat       statFormat;
    S9sFormat       nameFormat;
    S9sFormat       unitFormat;

    /*
     * First run-through: collecting some information.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap typeMap      = theList[idx].toVariantMap();
        S9sString     typeName     = typeMap["property_name"].toString();
        S9sString     description  = typeMap["description"].toString();
        S9sString     unit         = typeMap["unit"].toString();

        if (!options->isStringMatchExtraArguments(typeName))
            continue;

        if (unit.empty())
            unit = "-";

        nameFormat.widen(typeName);
        unitFormat.widen(unit);
    }

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
    {
        statFormat.widen("ST");
        nameFormat.widen("NAME");
        unitFormat.widen("UNIT");

        printf("%s", headerColorBegin());
         
        statFormat.printf("ST");
        nameFormat.printf("NAME");
        unitFormat.printf("UNIT");
        printf("DESCRIPTION");

        printf("%s", headerColorEnd());
        printf("\n");
    }

    /*
     * Printing the actual data.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap typeMap      = theList[idx].toVariantMap();
        S9sString     typeName     = typeMap["property_name"].toString();
        S9sString     unit         = typeMap["unit"].toString();
        S9sString     description  = typeMap["description"].toString();
        bool          isReadable   = typeMap["is_public"].toBoolean();
        bool          isWritable   = typeMap["is_writable"].toBoolean();
        S9sString     stat;
        int           nColumns    = 0;
    
        if (!options->isStringMatchExtraArguments(typeName))
            continue;

        if (unit.empty())
            unit = "-";

        if (description.empty())
            description = "-";

        stat += isReadable ? "r" : "-";
        stat += isWritable ? "w" : "-";

        nColumns += statFormat.realWidth();
        nColumns += nameFormat.realWidth();
        nColumns += unitFormat.realWidth();
        
        if (isTerminal && nColumns < terminalWidth)
        {
            int remaining  = terminalWidth - nColumns;
            
            if (remaining < (int) description.length())
            {
                description.resize(remaining - 1);
                description += "…";
            }
        }
        
        statFormat.printf(stat);
        printf("%s", propertyColorBegin());
        nameFormat.printf(typeName);
        printf("%s", propertyColorEnd());

        unitFormat.printf(unit);

        printf("%s", STR(description));
        printf("\n");
    }
}

/**
 * This function will print the properties of a type from the metatype system in
 * a brief list.
 */
void 
S9sRpcReply::printMetaTypePropertyListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("metatype_info").toVariantList();
    int             isTerminal    = options->isTerminal();
    int             terminalWidth = options->terminalWidth();
    S9sFormat       nameFormat;
    int             currentPosition = 0;

    /*
     * First run-through: collecting some information.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap typeMap      = theList[idx].toVariantMap();
        S9sString     typeName     = typeMap["property_name"].toString();
        
        if (!options->isStringMatchExtraArguments(typeName))
            continue;

        nameFormat.widen(typeName);
    }
    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap typeMap      = theList[idx].toVariantMap();
        S9sString     typeName     = typeMap["property_name"].toString();
        
        if (!options->isStringMatchExtraArguments(typeName))
            continue;

        printf("%s", propertyColorBegin());
        nameFormat.printf(typeName);
        printf("%s", propertyColorEnd());
        
        currentPosition += nameFormat.realWidth();
        if (currentPosition + nameFormat.realWidth() > terminalWidth ||
                !isTerminal)
        {
            printf("\n");
            currentPosition = 0;
        }
    }
    
    if (currentPosition > 0)
        printf("\n");
}
