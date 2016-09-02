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
#include "S9sRegExp"

#define DEBUG
#define WARNING
#include "s9sdebug.h"

bool
S9sRpcReply::isOk() const
{
    if (contains("requestStatus"))
        return at("requestStatus").toString().toLower() == "ok";

    return false;
}

S9sString
S9sRpcReply::errorString() const
{
    if (contains("errorString"))
        return at("errorString").toString();

    return S9sString();
}

void 
S9sRpcReply::printJobStarted()
{
    S9sString  status = operator[]("requestStatus").toString();
    int        id     = operator[]("jobId").toInt();

    //printf("%s", STR(toString()));
    if (status == "ok")
    {
        S9sVariantMap job = operator[]("job").toVariantMap();;

        if (job.empty())
        {
            printf("Job with ID %d registered.\n", id);
        } else {
            id = job["job_id"].toInt();

            printf("Job with ID %d registered.\n", id);
        }
    } else {
        printf("%s", STR(toString()));
    }
}

void
S9sRpcReply::printJobLog()
{
    S9sOptions     *options = S9sOptions::instance();
    S9sVariantList  theList = operator[]("messages").toVariantList();

    if (options->isJsonRequested())
    {
        printf("%s\n", STR(toString()));
        return;
    }

    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sVariantMap theMap = theList[idx].toVariantMap();
        S9sString     message = theMap["message_text"].toString();

        html2ansi(message);
        printf("%s\n", STR(message));
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

void 
S9sRpcReply::printClusterListLong()
{
    S9sOptions     *options = S9sOptions::instance();
    bool            syntaxHighlight = options->useSyntaxHighlight();
    S9sVariantList  theList = operator[]("clusters").toVariantList();

    printf("Total: %lu\n", theList.size());
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
        const char   *nameStart   = "";
        const char   *nameEnd     = "";

        if (syntaxHighlight)
        {
            nameStart = XTERM_COLOR_BLUE;
            nameEnd   = TERM_NORMAL;
        }
        
        printf("%4d ", clusterId); 
        printf("%6s ", STR(state));
        printf("%-8s ", STR(clusterType.toLower()));
        printf("%-12s ", STR(vendor + " " + version));
        printf("%s%s%s\n", nameStart, STR(clusterName), nameEnd);
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
        S9sString     text        = theMap["status_text"].toString();
        const char   *nameColor   = "";
        const char   *endColor    = "";

        if (syntaxHighlight)
        {
            nameColor = XTERM_COLOR_LIGHT_GREEN;
            endColor  = TERM_NORMAL;
        }

        printf("%4d %-14s %s%-20s%s %s\n", 
                clusterId, 
                STR(clusterType.toLower()),
                nameColor, STR(clusterName), endColor,
                STR(text));
    }
}

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

    printf("Total: %d\n", total);

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
}


void 
S9sRpcReply::printJobListLong()
{
    printf("TBD\n");
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
    s.replace("<em style='color: #c66211;'>", XTERM_COLOR_3);
    s.replace("<em style='color: #75599b;'>", XTERM_COLOR_3);
    s.replace("<strong style='color: #110679;'>", XTERM_COLOR_16);
    s.replace("<strong style='color: #59a449;'>", XTERM_COLOR_9);
    s.replace("<em style='color: #007e18;'>", XTERM_COLOR_4);
    s.replace("<em style='color: #7415f6;'>", XTERM_COLOR_5);
    s.replace("<em style='color: #1abc9c;'>", XTERM_COLOR_6);
    s.replace("<em style='color: #d35400;'>", XTERM_COLOR_7);
    s.replace("<em style='color: #c0392b;'>", XTERM_COLOR_8);

    //s.replace("", );
    s.replace("</em>",                        TERM_NORMAL);
    s.replace("</strong>",                    TERM_NORMAL);
#endif
}

