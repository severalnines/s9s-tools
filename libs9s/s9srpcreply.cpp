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
#include "s9srpcreply.h"

#include <stdio.h>

#include "S9sOptions"
#include "S9sDateTime"
#include "S9sFile"
#include "S9sFormat"
#include "S9sRegExp"
#include "S9sNode"
#include "S9sCluster"
#include "S9sBackup"
#include "S9sMessage"
#include "S9sCmonGraph"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

#define BoolToHuman(boolVal) ((boolVal) ? 'y' : 'n')

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

    if (isOk())
    {
        if (contains("errorString"))
        {
            // Reply message in RPC 1.0.
            printf("%s\n", STR(at("errorString").toString()));
        } else if (contains("error_string"))
        {
            // Reply message in RPC 2.0
            printf("%s\n", STR(at("error_string").toString()));
        } else if (contains("messages"))
        {
            // ROC 2.0 might hold multiple messages.
            S9sVariantList list = at("messages").toVariantList();
    
            for (uint idx = 0u; idx < list.size(); ++idx)
            {
                printf("%s\n", STR(list[idx].toString()));
            }
        } else {
            // Well, no message, everything is ok.
            printf("%s\n", STR(defaultMessage));
        }
    } else {
        if (contains("errorString"))
        {
            // Reply message in RPC 1.0.
            PRINT_ERROR("%s", STR(at("errorString").toString()));
        } else if (contains("error_string"))
        {
            // Reply message in RPC 2.0
            PRINT_ERROR("%s", STR(at("error_string").toString()));
        } else if (contains("messages"))
        {
            // ROC 2.0 might hold multiple messages.
            S9sVariantList list = at("messages").toVariantList();
    
            for (uint idx = 0u; idx < list.size(); ++idx)
            {
                PRINT_ERROR("%s", STR(list[idx].toString()));
            }
        } else {
            // Well, no message, everything is ok.
            PRINT_ERROR("Error: Unknown error.");
        }
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

void
S9sRpcReply::printProcessList()
{
    S9sOptions  *options = S9sOptions::instance();

    if (options->isJsonRequested())
    {
        printf("\n%s\n", STR(toString()));
        return;
    }

    if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
        return;
    }

    // FIXME: No detailed list format available for processes.
    printProcessListBrief();
}


void
S9sRpcReply::printProcessListBrief(
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
    printf("%s", STR(S9sString::space * columns));
    printf("%s", TERM_NORMAL);
    //printf("\n");

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
        
        printf("\n");
        //printf("%d ", maxLines);
        //printf("%u ", idx);

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

        printf(TERM_ERASE_EOL);

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
        return;
    }

    if (!isOk())
    {
        PRINT_ERROR("%s", STR(errorString()));
        return;
    }

    if (options->isLongRequested())
        printBackupListLong();
    else
        printBackupListBrief();
}

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

    if (options->isLongRequested())
        printUserListLong();
    else
        printUserListBrief();
}

void 
S9sRpcReply::printMaintenanceList()
{
    S9sOptions *options = S9sOptions::instance();
    
    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));

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
    else if (options->isStatRequested())
        printNodeListStat();
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
    else if (!isOk())
        PRINT_ERROR("%s", STR(errorString()));
    else if (options->isStatRequested())
        printClusterListStat();
    else if (options->isLongRequested())
        printClusterListLong();
    else
        printClusterListBrief();
}

void
S9sRpcReply::printConfigList()
{
    S9sOptions *options = S9sOptions::instance();

    if (options->isJsonRequested())
        printf("%s\n", STR(toString()));
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

    if (options->hasLogFormat())
        formatString = options->logFormat();

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
    S9sVariantList  theList   = operator[]("clusters").toVariantList();
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

/**
 * This method will print the reply as a detailed cluster list (aka 
 * "cluster --list --long").
 */
void 
S9sRpcReply::printClusterListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("clusters").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       requestedName = options->clusterName();
    int             terminalWidth = options->terminalWidth();
    S9sString       formatString = options->longClusterFormat();
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
    }

    /*
     * Printing the header.
     */
    if (!options->isNoHeaderRequested())
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
        S9sString     text        = clusterMap["status_text"].toString();
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

        if (nColumns < terminalWidth)
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
        printf("%s", userColorEnd());
        
        printf("%s", clusterColorBegin());
        nameFormat.printf(clusterName);
        printf("%s", clusterColorEnd());

        printf("%s\n", STR(statusText));
    }
   
    if (!options->isBatchRequested())
        printf("Total: %lu\n", (unsigned long int) theList.size());
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

    printf("%s", TERM_INVERSE/*headerColorBegin()*/);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL /*headerColorEnd()*/);
    
    //
    //
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
 * detail.
 * 
 * 
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
    // The title that is in inverse. 
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
    //
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
    
    printf("%s      IP:%s ", greyBegin, greyEnd);
    printf("%-16s ", STR(node.ipAddress()));
    //printf("\n");
    
    printf("          %sPort:%s ", greyBegin, greyEnd);
    if (node.hasPort())
    printf("%d ", node.port());
    printf("\n");
    
    // Line 
    printf("%s   Alias:%s ", greyBegin, greyEnd);
    printf("%-16s ", STR("'" + node.alias() + "'"));
    //printf("\n");
    
    printf("%s         Owner:%s ", greyBegin, greyEnd);
    printf("%s%s%s/%s%s%s ", 
            userColorBegin(), STR(cluster.ownerName()), userColorEnd(),
            groupColorBegin(cluster.groupOwnerName()), 
            STR(cluster.groupOwnerName()), 
            groupColorEnd());
    printf("\n");
    
    printf("%s   Class:%s ", greyBegin, greyEnd);
    printf("%s%-24s%s ", 
            typeColorBegin(), 
            STR(node.className()), 
            typeColorEnd());
    //printf("\n");
    
    printf("%s  Type:%s ", greyBegin, greyEnd);
    printf("%s", STR(node.nodeType()));
    printf("\n");
    
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

    // A line for the version.
    printf("%s Version:%s ", greyBegin, greyEnd);
    printf("%s", STR(node.version()));
    printf("\n");


    // A line for the humar readable message.
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
    printf("%20s", STR(S9sString::pastTime(node.lastSeen())));
    //printf("\n");
    
    printf("%s        SSH:%s ", greyBegin, greyEnd);
    printf("%d ", node.sshFailCount());
    printf("%sfail(s)%s ", greyBegin, greyEnd);

    printf("\n");

    //
    // A line of switches.
    //
    printf("%sConnected:%s %c ", 
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

    printf("\n\n");
}

void 
S9sRpcReply::printNodeListStat()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("clusters").toVariantList();

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
S9sRpcReply::printClusterListStat()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("clusters").toVariantList();

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
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sString       clusterNameFilter = options->clusterName();
    S9sVariantList  theList = operator[]("clusters").toVariantList();
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
            S9sString     version   = hostMap["version"].toString();
            int           port      = hostMap["port"].toInt(-1);
            int           clusterId = hostMap["clusterid"].toInt();

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
        printf("%s", headerColorEnd());

        printf("\n");
    }
        
    /*
     * Second run: doing the actual printing.
     */
    for (uint idx2 = 0; idx2 < hostList.size(); ++idx2)
    {
        S9sVariantMap hostMap   = hostList[idx2].toVariantMap();
        S9sNode       node      = hostMap;
        S9sString     hostName  = node.name();
        int           clusterId = hostMap["clusterid"].toInt();
        S9sString     status    = hostMap["hoststatus"].toString();
        S9sString     className = hostMap["class_name"].toString();
        //S9sString     nodeType  = hostMap["nodetype"].toString();
        S9sString     message   = hostMap["message"].toString();
        S9sString     version   = hostMap["version"].toString();
        S9sString     clusterName = hostMap["cluster_name"].toString();
        bool maintenance = hostMap["maintenance_mode_active"].toBoolean();
        int           port      = hostMap["port"].toInt(-1);
            
        // Filtering...
        if (!options->isStringMatchExtraArguments(hostName))
            continue;

        if (message.empty())
            message = "-";

        if (version.empty())
            version = "-";

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
        nColumns  = 3 + 1;
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
    S9sVariantList  theList         = operator[]("jobs").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();
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
        int            jobId  = theMap["job_id"].toInt();
        int            cid    = theMap["cluster_id"].toInt();
        S9sString      user   = theMap["user_name"].toString();
        S9sString      group  = theMap["group_name"].toString();
        S9sString      status = theMap["status"].toString();
        S9sDateTime    timeStamp;
        S9sString      timeStampString;
        
        // Filtering.
        if (options->hasJobId() && options->jobId() != jobId)
            continue;

        if (group.empty())
            group = "-";

        // The timestamp. Now we use 'created' later we can make this
        // configurable.
        timeStamp.parse(theMap["created"].toString());
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
        int            jobId  = theMap["job_id"].toInt();
        int            cid    = theMap["cluster_id"].toInt();
        S9sString      status = theMap["status"].toString();
        S9sString      title  = theMap["title"].toString();
        S9sString      user   = theMap["user_name"].toString();
        S9sString      group  = theMap["group_name"].toString();
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
    S9sVariantList  theList         = operator[]("jobs").toVariantList();
    bool            syntaxHighlight = options->useSyntaxHighlight();
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
        S9sVariantMap  theMap = theList[idx].toVariantMap();
        int            jobId  = theMap["job_id"].toInt();
        S9sString      user   = theMap["user_name"].toString();
        S9sString      status = theMap["status"].toString();
        
        // Filtering.
        if (options->hasJobId() && options->jobId() != jobId)
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
        int            jobId      = theMap["job_id"].toInt();
        S9sString      status     = theMap["status"].toString();
        S9sString      title      = theMap["title"].toString();
        S9sString      statusText = theMap["status_text"].toString();
        S9sString      statusTextMonochrome;
        S9sString      user       = theMap["user_name"].toString();
        S9sString      group      = theMap["group_name"].toString();
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

        // Filtering.
        if (options->hasJobId() && options->jobId() != jobId)
            continue;

        // The title.
        if (title.empty())
            title = "Untitled Job";

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
        
        printf("\n");
        
        if (!scheduled.empty())
        {
            printf("%sScheduled :%s %s%19s%s\n", 
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
 * Prints the list of backups in its brief format.
 */
void 
S9sRpcReply::printBackupListBrief()
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
            S9sVariantMap  backup  = backups[idx2].toVariantMap();
            S9sVariantList files   = backup["files"].toVariantList();

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
S9sRpcReply::printBackupListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sString       formatString = options->longBackupFormat();
    S9sVariantList  dataList;
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sFormat       sizeFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       idFormat;
    S9sFormat       cidFormat;
    S9sFormat       stateFormat;
    S9sFormat       createdFormat;
    S9sFormat       ownerFormat;
    const char     *colorBegin = "";
    const char     *colorEnd   = "";
   
    if (options->hasBackupFormat())
        formatString = options->backupFormat();

    // One is RPC 1.0, the other is 2.0.
    if (contains("data"))
        dataList = operator[]("data").toVariantList();
    else if (contains("backup_records"))
        dataList = operator[]("backup_records").toVariantList();
   
    /*
     * If there is a format string we simply print the list using that format.
     */
    if (!formatString.empty())
    {
        for (uint idx = 0; idx < dataList.size(); ++idx)
        {
            S9sVariantMap  theMap    = dataList[idx].toVariantMap();
            S9sBackup      backup    = theMap;
            
            for (int backupIdx = 0; backupIdx < backup.nBackups(); ++backupIdx)
            {
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

        return;
    }

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
        S9sString      owner     = backup.configOwner();
        int            id        = backup.id(); 
        S9sString      status    = backup.status(); 

        cidFormat.widen(clusterId);
        stateFormat.widen(status);
        hostNameFormat.widen(hostName);
        ownerFormat.widen(owner);
        
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

            for (int fileIdx = 0; fileIdx < backup.nFiles(backupIdx); ++fileIdx)
            {
                ulonglong   size = backup.fileSize(backupIdx, fileIdx).toULongLong();
                S9sString   sizeString;
                S9sString   createdString = backup.fileCreated(backupIdx, fileIdx).toString();
                S9sDateTime created;
        
                created.parse(createdString);
                createdString = options->formatDateTime(created);
                
                sizeString = S9sFormat::toSizeString(size);
                
                createdFormat.widen(createdString);
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
        cidFormat.widen("CID");
        stateFormat.widen("STATE");
        ownerFormat.widen("OWNER");
        hostNameFormat.widen("HOSTNAME");
        createdFormat.widen("CREATED");
        sizeFormat.widen("SIZE");

        printf("%s", headerColorBegin());
        idFormat.printf("ID");
        cidFormat.printf("CID");
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
        //S9sVariantMap  configMap = theMap["config"].toVariantMap();
        S9sString      owner     = backup.configOwner();
        int            id        = backup.id();
        S9sString      status    = backup.status();
        S9sString      root      = backup.rootDir();

        if (backups.size() == 0u)
        {
            S9sString     path          = "-";
            S9sString     sizeString    = "-";
            S9sString     createdString = "-";
                
            idFormat.printf(id);
            cidFormat.printf(clusterId);
            stateFormat.printf(status);
            ownerFormat.printf(owner);
            hostNameFormat.printf(hostName);
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
                ulonglong   size = backup.fileSize(backupIdx, fileIdx).toULongLong();
                S9sString   sizeString;
                S9sString   createdString = backup.fileCreated(backupIdx, fileIdx).toString();
                S9sDateTime created;

                if (options->fullPathRequested())
                {
                    if (!root.endsWith("/"))
                        root += "/";

                    path = root + path;
                }

                created.parse(createdString);
                createdString = options->formatDateTime(created);

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
                cidFormat.printf(clusterId);
                stateFormat.printf(status);

                printf("%s", userColorBegin());
                ownerFormat.printf(owner);
                printf("%s", userColorEnd());

                hostNameFormat.printf(hostName);
                createdFormat.printf(createdString);
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
S9sRpcReply::printUserListBrief()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  userList = operator[]("users").toVariantList();
    int             authUserId = operator[]("request_user_id").toInt();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    bool            whoAmIRequested = options->isWhoAmIRequested();
    const char     *colorBegin = "";
    const char     *colorEnd   = "";

    userList = operator[]("users").toVariantList();

    /*
     * 
     */
    for (uint idx = 0; idx < userList.size(); ++idx)
    {
        S9sVariantMap  userMap    = userList[idx].toVariantMap();
        S9sString      userName   = userMap["user_name"].toString();
        int            userId     = userMap["user_id"].toInt();
        
        //
        // Filtering.
        //
        if (whoAmIRequested && userId != authUserId)
            continue;

        if (!options->isStringMatchExtraArguments(userName))
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
    S9sVariantList  userList = operator[]("users").toVariantList();
    int             authUserId = operator[]("request_user_id").toInt();
    bool            whoAmIRequested = options->isWhoAmIRequested();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    const char     *colorBegin = "";
    const char     *colorEnd   = "";
    const char     *groupColorBegin = "";
    const char     *groupColorEnd   = "";
    S9sFormat       idFormat;
    S9sFormat       userNameFormat;
    S9sFormat       groupNamesFormat;
    S9sFormat       emailFormat;

    userList = operator[]("users").toVariantList();
   
    /*
     * Going through first and collecting some informations.
     */
    for (uint idx = 0; idx < userList.size(); ++idx)
    {
        S9sVariantMap  userMap    = userList[idx].toVariantMap();
        S9sVariantList groupList  = userMap["groups"].toVariantList();
        S9sString      userName   = userMap["user_name"].toString();
        S9sString      emailAddress = userMap["email_address"].toString();
        int            userId     = userMap["user_id"].toInt();
        S9sString      groupNames; 

        /*
         * Filtering.
         */
        if (whoAmIRequested && userId != authUserId)
            continue;
        
        if (!options->isStringMatchExtraArguments(userName))
            continue;

        /*
         * Groups.
         */
        for (uint idx = 0u; idx < groupList.size(); ++idx)
        {
            S9sVariantMap groupMap  = groupList[idx].toVariantMap();
            S9sString     groupName = groupMap["group_name"].toString();

            if (!groupNames.empty())
                groupNames += ",";

            groupNames += groupName;
        }

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
        S9sVariantMap  userMap    = userList[idx].toVariantMap();
        S9sVariantList groupList  = userMap["groups"].toVariantList();
        S9sString      userName   = userMap["user_name"].toString();
        int            userId     = userMap["user_id"].toInt();
        S9sString      title      = userMap["title"].toString();
        S9sString      firstName  = userMap["first_name"].toString();
        S9sString      lastName   = userMap["last_name"].toString();
        S9sString      emailAddress = userMap["email_address"].toString();
        S9sString      fullName;
        S9sString      groupNames; 
       
        //
        // Filtering.
        //
        if (whoAmIRequested && userId != authUserId)
            continue;
        
        if (!options->isStringMatchExtraArguments(userName))
            continue;

        //
        // Concatenating the group names into one string.
        //
        for (uint idx = 0u; idx < groupList.size(); ++idx)
        {
            S9sVariantMap groupMap  = groupList[idx].toVariantMap();
            S9sString     groupName = groupMap["group_name"].toString();

            if (!groupNames.empty())
                groupNames += ",";

            groupNames += groupName;
        }
        
        if (groupNames.empty())
            groupNames = "-";
        
        if (emailAddress.empty())
            emailAddress = "-";

        /*
         * Concatenating the real-world names.
         */
        if (!title.empty())
        {
            if (!fullName.empty())
                fullName += " ";

            fullName += title;
        }

        if (!firstName.empty())
        {
            if (!fullName.empty())
                fullName += " ";

            fullName += firstName;
        }

        if (!lastName.empty())
        {
            if (!fullName.empty())
                fullName += " ";

            fullName += lastName;
        }

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
    
    printf("\n");
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

    printf("\n");
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

    printf("\n");
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
    S9sVariantList  theList = operator[]("clusters").toVariantList();
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
        if (state == "FAILURE")
            return XTERM_COLOR_RED;
        if (state == "STARTED")
            return XTERM_COLOR_GREEN;
        if (state == "STOPPED")
            return XTERM_COLOR_YELLOW;
        if (state == "SHUTTING_DOWN")
            return XTERM_COLOR_YELLOW;
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
        else if (fileName.endsWith(".ini"))
            return XTERM_COLOR_LIGHT_PURPLE;
        else if (fileName.endsWith(".pid"))
            return XTERM_COLOR_LIGHT_RED;
        else
            return TERM_NORMAL;
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
        
        if (!options->isStringMatchExtraArguments(typeName))
            continue;

        nameFormat.widen(typeName);
    }
    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap typeMap      = theList[idx].toVariantMap();
        S9sString     typeName     = typeMap["type_name"].toString();
        S9sString     description  = typeMap["description"].toString();

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


void 
S9sRpcReply::printMetaTypePropertyListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("metatype_info").toVariantList();
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
        nameFormat.widen("NAME");
        unitFormat.widen("UNIT");

        printf("%s", headerColorBegin());
         
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
        
        if (!options->isStringMatchExtraArguments(typeName))
            continue;

        if (unit.empty())
            unit = "-";

        if (description.empty())
            description = "-";

        printf("%s", propertyColorBegin());
        nameFormat.printf(typeName);
        printf("%s", propertyColorEnd());

        unitFormat.printf(unit);

        printf("%s", STR(description));
        printf("\n");
    }
}

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
