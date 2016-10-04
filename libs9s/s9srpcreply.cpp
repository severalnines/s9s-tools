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
#include "S9sFormat"
#include "S9sRegExp"
#include "S9sNode"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

/**
 * \returns true if the reply states that the request status is 'ok'.
 */
bool
S9sRpcReply::isOk() const
{
    if (contains("requestStatus"))
        return at("requestStatus").toString().toLower() == "ok";

    return false;
}

/**
 * \returns The error string sent by the controller in the reply if there is
 *   indeed an error string.
 */
S9sString
S9sRpcReply::errorString() const
{
    if (contains("errorString"))
        return at("errorString").toString();

    return S9sString();
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

    retval.clear();

    if (job.empty())
        return false;

    // The job id.
    jobId = job["job_id"].toInt();
    tmp.sprintf("Job %d ", jobId);
    retval += tmp;

    // The status
    status = job["status"].toString();
    tmp.sprintf("%-10s ", STR(status));
    if (syntaxHighlight)
    {
        if (status == "RUNNING" || status == "FINISHED")
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

    retval += job["status_text"].toString();
    retval += "      ";
    
    if (syntaxHighlight)
        retval += TERM_NORMAL;

    return 
        status == "ABORTED"  ||
        status == "FINISHED" ||
        status == "FAILED";
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
    S9sString       status = operator[]("requestStatus").toString();
    int             id;

    if (options->isJsonRequested())
    {
        printf("%s", STR(toString()));
        return;
    }

    if (status == "ok")
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

bool 
compareProcessByCpuUsage(
        const S9sVariant &a,
        const S9sVariant &b)
{
    S9sVariantMap aMap = a.toVariantMap();
    S9sVariantMap bMap = b.toVariantMap();

    return aMap["cpu_usage"].toDouble() > bMap["cpu_usage"].toDouble();
}

void
S9sRpcReply::printProcessList()
{
    S9sVariantList  hostList = operator[]("data").toVariantList();
    S9sVariantList  processList;

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

    // rss          resident set size
    // vsz          virtual memory size
    for (uint idx = 0u; idx < processList.size(); ++idx)
    {
        S9sVariantMap process = processList[idx].toVariantMap();
        S9sString     hostName = process["hostname"].toString();
        S9sString     user = process["user"].toString();
        int           pid = process["pid"].toInt();
        S9sString     executable = process["executable"].toString();
        double        cpuUsage = process["cpu_usage"].toDouble();
        double        memUsage = process["mem_usage"].toDouble();
        S9sString     state = process["state"].toString();
        ulonglong     rss = process["res_mem"].toULongLong();
        ulonglong     virtMem = process["virt_mem"].toULongLong();
        int           priority = process["priority"].toInt();

        rss     /= 1024;
        virtMem /= 1024;
        printf("%6d %8s %12s %4d %6.2f %6.2f %8llu %8llu %1s %s\n", 
                pid, STR(user), STR(hostName), 
                priority,
                cpuUsage, memUsage,
                virtMem, rss,
                STR(state), STR(executable));
    }
}

/**
 * Prints the job log in its short format. In this format only the messages are
 * printed.
 */
void
S9sRpcReply::printJobLogBrief()
{
    S9sVariantList  theList = operator[]("messages").toVariantList();

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap  = theList[idx].toVariantMap();
        S9sString     message = theMap["message_text"].toString();

        html2ansi(message);
        printf("%s\n", STR(message));
    }
}

/**
 * Prints the job log in its long format (aka "job --log --list").
 */
void
S9sRpcReply::printJobLogLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("messages").toVariantList();
    int             terminalWidth = options->terminalWidth();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       line = S9sString::dash * terminalWidth;
    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap          = theList[idx].toVariantMap();
        S9sString      message         = theMap["message_text"].toString();
        S9sString      status          = theMap["message_status"].toString();
        S9sString      created         = theMap["created"].toString();
        const char    *stateColorStart = "";
        const char    *stateColorEnd   = "";
  
        html2ansi(message);
        if (!created.empty())
        {
            S9sDateTime tmp;

            tmp.parse(created);
            created = tmp.toString(S9sDateTime::MySqlLogFileFormat);
        }
        
        if (syntaxHighlight)
        {
            if (status == "JOB_SUCCESS")
            {
                stateColorStart = XTERM_COLOR_GREEN;
                stateColorEnd   = TERM_NORMAL;
            } else if (status == "JOB_WARNING")
            {
                stateColorStart = XTERM_COLOR_YELLOW;
                stateColorEnd   = TERM_NORMAL;
            } else if (status == "JOB_FAILED")
            {
                stateColorStart = XTERM_COLOR_RED;
                stateColorEnd   = TERM_NORMAL;
            }
        }

        //printf("%s%s%s\n\n", TERM_BOLD, STR(message), TERM_NORMAL);
        printf("%s\n\n", STR(message));

        printf("  %sCreated:%s %s%s%s  ", 
                XTERM_COLOR_DARK_GRAY, 
                TERM_NORMAL,
                XTERM_COLOR_LIGHT_GRAY,
                STR(created),
                TERM_NORMAL); 
        
        printf("%sStatus:%s %s%s%s\n", 
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

/**
 * Generic method that prints the reply as a node list.
 */
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
    else if (options->isLongRequested())
        printClusterListLong();
    else
        printClusterListBrief();
}

/**
 * Private low-level function to print the cluster list in the "brief" format.
 * The brief format can be used in shell scripts to create lists to walk
 * through.
 */
void 
S9sRpcReply::printClusterListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("clusters").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    int             nPrinted = 0;

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap = theList[idx].toVariantMap();
        S9sString     clusterName = theMap["cluster_name"].toString();

        if (syntaxHighlight)
            printf("%s%s%s ", TERM_BLUE, STR(clusterName), TERM_NORMAL);
        else
            printf("%s ", STR(clusterName));

        ++nPrinted;
    }

    if (nPrinted > 0)
    {
        printf("\n");
        fflush(stdout);
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
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("clusters").toVariantList();
    S9sString       requestedName = options->clusterName();
    int             terminalWidth = options->terminalWidth();
    S9sFormat       idFormat;

    /*
     * First run-through: collecting some information.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap      = theList[idx].toVariantMap();
        int           clusterId   = theMap["cluster_id"].toInt();

        idFormat.widen(clusterId);
    }

    /*
     * Second run: doing the actual printing.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap      = theList[idx].toVariantMap();
        S9sString     clusterName = theMap["cluster_name"].toString();
        int           clusterId   = theMap["cluster_id"].toInt();
        S9sString     clusterType = theMap["cluster_type"].toString();
        S9sString     state       = theMap["state"].toString();
        S9sString     text        = theMap["status_text"].toString();
        S9sString     vendor      = theMap["vendor"].toString();
        S9sString     version     = theMap["version"].toString();
        S9sString     statusText  = theMap["status_text"].toString();
        int           nColumns    = 0;
        const char   *nameStart   = "";
        const char   *nameEnd     = "";

        if (!requestedName.empty() && requestedName != clusterName)
            continue;

        // This should not happen!
        if (clusterName.empty())
            continue;

        if (syntaxHighlight)
        {
            nameStart = XTERM_COLOR_BLUE;
            nameEnd   = TERM_NORMAL;
        }
        
        nColumns += idFormat.realWidth();
        nColumns +=  7;
        nColumns +=  9;
        nColumns += 12;
        nColumns += 10;

        if (nColumns < terminalWidth)
        {
            int remaining = terminalWidth - nColumns;
            
            if (remaining < (int) statusText.length())
            {
                statusText.resize(remaining - 1);
                statusText += "…";
            }
        }

        idFormat.printf(clusterId); 
        printf("%6s ", STR(state));
        printf("%-8s ", STR(clusterType.toLower()));
        printf("%-12s ", STR(vendor + " " + version));
        printf("%s%s%s ", nameStart, STR(clusterName), nameEnd);
        printf("%s\n", STR(statusText));
    }
   
    if (!options->isBatchRequested())
        printf("Total: %lu\n", theList.size());
}

/**
 * Prints the node list from the reply if the reply contains a node list. The
 * printed list is in "brief" format.
 */
void 
S9sRpcReply::printNodeListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("clusters").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    int             nPrinted = 0;
    uint            maxHostNameLength = 0u;
    S9sString       hostNameFormat;
    int             terminalWidth = options->terminalWidth();
    int             nColumns;
    int             column = 0;
    
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
            
            if (hostName.length() > maxHostNameLength)
                maxHostNameLength = hostName.length();
        }
    }

    nColumns          = terminalWidth / (maxHostNameLength + 1);
    maxHostNameLength = terminalWidth / nColumns;

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

            if (syntaxHighlight)
            {
                if (status == "CmonHostRecovery" || 
                        status == "CmonHostShutDown")
                {
                    nameStart = XTERM_COLOR_YELLOW;
                    nameEnd   = TERM_NORMAL;
                } else if (status == "CmonHostUnknown" ||
                        status == "CmonHostOffLine")
                {
                    nameStart = XTERM_COLOR_RED;
                    nameEnd   = TERM_NORMAL;
                } else {
                    nameStart = XTERM_COLOR_GREEN;
                    nameEnd   = TERM_NORMAL;
                }
            }
                    
            printf(STR(hostNameFormat), 
                    nameStart, STR(hostName), nameEnd);

            column += maxHostNameLength;
            if (column + (int) maxHostNameLength > terminalWidth)
            {
                printf("\n");
                column = 0;
            }

            ++nPrinted;
        }
    }

    if (nPrinted > 0)
    {
        printf("\n");
        fflush(stdout);
    }
}

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


/**
 * Prints the node list in its long format (aka "node --list --long).
 */
void 
S9sRpcReply::printNodeListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       clusterNameFilter = options->clusterName();
    S9sVariantList  theList = operator[]("clusters").toVariantList();
    S9sVariantList  hostList;
    uint            maxHostNameLength = 0u;
    S9sString       hostNameFormat;
    uint            maxVersionLength  = 0u;
    S9sString       versionFormat;
    uint            maxClusterNameLength = 0u;
    S9sString       clusterNameFormat;
    int             total = 0;
    int             terminalWidth = options->terminalWidth();
    int             nColumns;

    /*
     * First run-through: collecting some information.
     */
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        S9sVariantList hosts = theMap["hosts"].toVariantList();
        S9sString      clusterName = theMap["cluster_name"].toString();
        
        total += hosts.size();

        if (!clusterNameFilter.empty() && clusterNameFilter != clusterName)
            continue;

        if (clusterName.length() > maxClusterNameLength)
            maxClusterNameLength = clusterName.length();

        for (uint idx2 = 0; idx2 < hosts.size(); ++idx2)
        {
            S9sVariantMap hostMap   = hosts[idx2].toVariantMap();
            S9sNode       node      = hostMap;
            S9sString     hostName  = node.name();
            S9sString     version   = hostMap["version"].toString();
            
            if (hostName.length() > maxHostNameLength)
                maxHostNameLength = hostName.length();

            if (version.length() > maxVersionLength)
                maxVersionLength = version.length();
        
            hostMap["cluster_name"] = clusterName;
            hostList << hostMap;
        }
    }

    hostNameFormat.sprintf("%%s%%-%us%%s ", maxHostNameLength);
    versionFormat.sprintf("%%-%us ", maxVersionLength);
    clusterNameFormat.sprintf("%%-%us ", maxClusterNameLength);

    /*
     * Sorting the hosts.
     */
    sort(hostList.begin(), hostList.end(), compareHostMaps);

    /*
     * Second run: doing the actual printing.
     */
    for (uint idx2 = 0; idx2 < hostList.size(); ++idx2)
    {
        S9sVariantMap hostMap   = hostList[idx2].toVariantMap();
        S9sNode       node      = hostMap;
        S9sString     hostName  = node.name();
        S9sString     status    = hostMap["hoststatus"].toString();
        S9sString     className = hostMap["class_name"].toString();
        S9sString     nodeType  = hostMap["nodetype"].toString();
        S9sString     message   = hostMap["message"].toString();
        S9sString     version   = hostMap["version"].toString();
        S9sString     clusterName = hostMap["cluster_name"].toString();
        bool maintenance = hostMap["maintenance_mode_active"].toBoolean();
        int           port      = hostMap["port"].toInt(-1);
        const char   *nameStart = "";
        const char   *nameEnd   = "";

        if (message.empty())
            message = "-";

        if (syntaxHighlight)
        {
            if (status == "CmonHostRecovery" || 
                    status == "CmonHostShutDown")
            {
                nameStart = XTERM_COLOR_YELLOW;
                nameEnd   = TERM_NORMAL;
            } else if (status == "CmonHostUnknown" ||
                    status == "CmonHostOffLine")
            {
                nameStart = XTERM_COLOR_RED;
                nameEnd   = TERM_NORMAL;
            } else {
                nameStart = XTERM_COLOR_GREEN;
                nameEnd   = TERM_NORMAL;
            }
        }

        // Calculating how much space we have for the message column.
        nColumns  = 3 + 1;
        nColumns += maxVersionLength + 1;
        nColumns += maxClusterNameLength + 1;
        nColumns += maxHostNameLength + 1;
        nColumns += 4 + 1;

        if (nColumns < terminalWidth)
        {
            int remaining = terminalWidth - nColumns;
            
            if (remaining < (int) message.length())
            {
                message.resize(remaining - 1);
                message += "…";
            }
        }

        printf("%s", STR(nodeTypeFlag(className, nodeType)));
        printf("%s", STR(nodeStateFlag(status)));
        printf("%c ", maintenance ? 'M' : '-');

        printf(STR(versionFormat), STR(version));
        printf(STR(clusterNameFormat), STR(clusterName));

        printf(STR(hostNameFormat), nameStart, STR(hostName), nameEnd);

        if (port >= 0)
            printf("%4d ", port);
        else
            printf("   - ");


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
    S9sVariantList  theList         = operator[]("jobs").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    int             total           = operator[]("total").toInt();
    unsigned int    userNameLength  = 0;
    S9sString       userNameFormat;
    unsigned int    statusLength  = 0;
    S9sString       statusFormat;


    //
    // The width of certain columns are variable.
    //
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap = theList[idx].toVariantMap();
        S9sString     user   = theMap["user_name"].toString();
        S9sString     status = theMap["status"].toString();

        if (user.length() > userNameLength)
            userNameLength = user.length();
        
        if (status.length() > statusLength)
            statusLength = status.length();
    }

    userNameFormat.sprintf("%%-%us ", userNameLength);
    statusFormat.sprintf("%%s%%-%ds%%s ", statusLength);

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

        // The title.
        if (title.empty())
            title = "Untitled Job";

        // The user name or if it is not there the user ID.
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
            percent = "  0%";
        }

        // The timestamp.
        created.parse(theMap["created"].toString());
        timeStamp = created.toString(S9sDateTime::MySqlLogFileFormat);

        if (syntaxHighlight)
        {
            if (status == "RUNNING" || status == "RUNNING_EXT")
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

        printf("%5d ", jobId);
        printf(STR(statusFormat), stateColorStart, STR(status), stateColorEnd);
        printf(STR(userNameFormat), STR(user));
        printf("%s ", STR(timeStamp));
        printf("%s ", STR(percent));
        printf("%s\n", STR(title));
    }
    
    printf("Total: %d\n", total);
}

/**
 * Prints the list of jobs in their detailed form.
 */
void 
S9sRpcReply::printJobListLong()
{
    S9sOptions     *options         = S9sOptions::instance();
    int             terminalWidth   = options->terminalWidth();
    S9sVariantList  theList         = operator[]("jobs").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    int             total           = operator[]("total").toInt();
    unsigned int    userNameLength  = 0;
    S9sString       userNameFormat;
    unsigned int    statusLength  = 0;
    S9sString       statusFormat;

    //
    // The width of certain columns are variable.
    //
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap = theList[idx].toVariantMap();
        S9sString     user   = theMap["user_name"].toString();
        S9sString     status = theMap["status"].toString();

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
        int            jobId      = theMap["job_id"].toInt();
        S9sString      status     = theMap["status"].toString();
        S9sString      title      = theMap["title"].toString();
        S9sString      statusText = theMap["status_text"].toString();
        S9sString      user       = theMap["user_name"].toString();
        S9sString      hostName   = theMap["ip_address"].toString();
        S9sString      created    = theMap["created"].toString();
        S9sString      ended      = theMap["ended"].toString();
        S9sString      started    = theMap["started"].toString();
        S9sString      scheduled  = theMap["scheduled"].toString();
        S9sString      bar;
        double         percent;
        S9sString      timeStamp;
        const char    *stateColorStart = "";
        const char    *stateColorEnd   = "";

        // The title.
        if (title.empty())
            title = "Untitled Job";

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
            if (status == "RUNNING" || status == "RUNNING_EXT")
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

        for (int n = 0; n < terminalWidth; ++n)
            printf("-");
        printf("\n");

        printf("%s%s%s\n", TERM_BOLD, STR(title), TERM_NORMAL);
        //printf("%s%s%s", XTERM_COLOR_LIGHT_GRAY, STR(statusText), TERM_NORMAL);
        printf("%s", STR(statusText));

        for (int n = statusText.length(); n < terminalWidth - 13; ++n)
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

        
        /*
         *
         */
        for (int n = 10; n < terminalWidth; ++n)
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
        printf("%sCreated   :%s %s%s%s    ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_LIGHT_GRAY, STR(created), TERM_NORMAL);

        printf("%sID   :%s %s%-10d%s ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_BLUE, jobId, TERM_NORMAL);

        printf("%sStatus :%s %s%s%s ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                stateColorStart, STR(status), stateColorEnd);

        printf("\n");

        printf("%sStarted   :%s %s%s%s    ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_LIGHT_GRAY, STR(started), TERM_NORMAL);
        
        printf("%sUser :%s %s%-10s%s ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_BLUE, STR(user), TERM_NORMAL);

        printf("%sHost   :%s %s%s%s ", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_BLUE, STR(hostName), TERM_NORMAL);
        
        printf("\n");



        printf("%sEnded     :%s %s%s%s\n", 
                XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                XTERM_COLOR_LIGHT_GRAY, STR(ended), TERM_NORMAL);
        
        if (!scheduled.empty())
        {
            printf("%sScheduled :%s %s%s%s\n", 
                    XTERM_COLOR_DARK_GRAY, TERM_NORMAL,
                    XTERM_COLOR_LIGHT_GRAY, STR(scheduled), TERM_NORMAL);
        }


        S9S_UNUSED(jobId);
        S9S_UNUSED(stateColorEnd);
        S9S_UNUSED(stateColorStart);
    }
        
    for (int n = 0; n < terminalWidth; ++n)
        printf("-");
    printf("\n");
    
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
    S9sString retval;
    int       nBars;
    int       remain;

    if (percent < 0.0)
        percent = 0.0;
    else if (percent > 100.0)
        percent = 100.0;

    nBars   = percent / 10;
    remain  = (int) percent % 10;
    S9S_WARNING("*** remain: %d", remain);

    retval += "[";

    if (syntaxHighlight)
        retval += XTERM_COLOR_BLUE;

    for (int n = 1; n <= nBars; ++n)
        retval += "█";

    if (percent < 100.0)
    {
        switch (remain)
        {
            case 0:
                retval += " ";
                break;

        	case 1:
                retval += "▏";
                break;

        	case 2:
                retval += "▎";
                break;

        	case 3:
                retval += "▍";
                break;

        	case 4:
                retval += "▌";
                break;

        	case 5:
                retval += "▋";
                break;

        	case 6:
        	case 7:
                retval += "▊";
                break;

        	case 8:
        	case 9:
                retval += "▉";
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

void 
S9sRpcReply::html2ansi(
        S9sString &s)
{
#if 0
    //
    // This is using a palette. Right now it seems to be a bit overcomplicated
    // to use a palette like this.
    //
    S9sRegExp regexp1("<em style='color: #([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f]);'>", "i");
    S9sRegExp regexp2("<strong style='color: #([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f]);'>", "i");

    s.replace(regexp1, "\033]4;1;rgb:$1/$2/$3\033\\\033[31m");
    s.replace(regexp2, "\033]4;1;rgb:$1/$2/$3\033\\\033[31m");
    
    s.replace("</em>",     "\e[m");
    s.replace("</strong>", "\e[m");
#else
    s.replace("<em style='color: #c66211;'>",     XTERM_COLOR_3);
    s.replace("<em style='color: #75599b;'>",     XTERM_COLOR_3);
    s.replace("<strong style='color: #110679;'>", XTERM_COLOR_16);
    s.replace("<strong style='color: #59a449;'>", XTERM_COLOR_9);
    s.replace("<em style='color: #007e18;'>",     XTERM_COLOR_4);
    s.replace("<em style='color: #7415f6;'>",     XTERM_COLOR_5);
    s.replace("<em style='color: #1abc9c;'>",     XTERM_COLOR_6);
    s.replace("<em style='color: #d35400;'>",     XTERM_COLOR_7);
    s.replace("<em style='color: #c0392b;'>",     XTERM_COLOR_8);
    s.replace("<em style='color: #0b33b5;'>",     XTERM_COLOR_BLUE);
    s.replace("<em style='color: #34495e;'>",     XTERM_COLOR_CYAN);
    s.replace("<strong style='color: red;'>",     XTERM_COLOR_RED);

    //s.replace("", );
    s.replace("</em>",                        TERM_NORMAL);
    s.replace("</strong>",                    TERM_NORMAL);

    // Replacing all the other colors. This code is originally created to be
    // used with a palette, but I am not sure if we should modify the palette,
    // so it is kinda unfinished here.
    S9sRegExp regexp1("<em style='color: #([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f]);'>", "i");
    S9sRegExp regexp2("<strong style='color: #([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f]);'>", "i");

    s.replace(regexp1, XTERM_COLOR_ORANGE);
    s.replace(regexp2, XTERM_COLOR_ORANGE);

    s.replace("<BR/>", "\n");
    s.replace("<br/>", "\n");
#endif
}

S9sString 
S9sRpcReply::nodeTypeFlag(
        const S9sString &className,
        const S9sString &nodeType)
{
    if (nodeType == "controller")
        return "c";
    else if (nodeType == "galera")
        return "g";
    else if (nodeType == "maxscale")
        return "x";
    else if (nodeType == "keepalived")
        return "k";
    else if (nodeType == "postgres")
        return "p";
    else if (nodeType == "mongo")
        return "m";
    else if (nodeType == "memcached")
        return "e";
    else if (nodeType == "proxysql")
        return "y";
    else if (nodeType == "haproxy")
        return "h";

    if (className == "CmonMySqlHost")
        return "s";
    
    return "?";
}

S9sString 
S9sRpcReply::nodeStateFlag(
        const S9sString &state)
{
    if (state == "CmonHostUnknown")
        return "?";
    else if (state == "CmonHostOnline")
        return "o";
    else if (state == "CmonHostOffLine")
        return "l";
    else if (state == "CmonHostFailed")
        return "f";
    else if (state == "CmonHostRecovery")
        return "r";
    else if (state == "CmonHostShutDown")
        return "-";

    return "?";
}

