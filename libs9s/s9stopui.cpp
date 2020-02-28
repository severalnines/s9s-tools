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
#include "S9sMutexLocker"
#include "S9sSqlProcess"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define DEBUG
#define WARNING
#include "s9sdebug.h"
        
struct termios orig_termios;

S9sTopUi::S9sTopUi(
        S9sRpcClient       &client,
        S9sTopUi::ViewMode  viewMode) :
    S9sDisplay(true),
    m_viewMode(viewMode),
    m_client(client),
    m_nReplies(0),
    m_clustersReplyReceived(0),
    m_clusterId(0),
    m_sortOrder(CpuUsage),
    m_communicating(false),
    m_viewDebug(false),
    m_reloadRequested(false)
{
}

S9sTopUi::~S9sTopUi()
{
}

/**
 * \param key The code of the pressed key.
 *
 * This function is called when the user pressed a key on the keyboard.
 */
void
S9sTopUi::processKey(
        int key)
{
    switch (key)
    {
        case 'q':
        case 'Q':
        case 0x1b:
        case 3:
            exit(0);
            break;

        case 'p':
        case 'P':
            m_sortOrder = PidOrder;
            break;

        case 'm':
        case 'M':
            m_sortOrder = MemUsage;
            break;
        
        case 'c':
        case 'C':
            m_sortOrder = CpuUsage;
            break;

        case 'd':
        case 'D':
            // Turning on and off the debug mode.
            m_viewDebug = !m_viewDebug;
            break;
    }
}

/**
 * \param button The mouse button code.
 * \param x The x coordinate measured in characters.
 * \param y The y coordinate measured in characters.
 * \returns True if the mouse event is processed and should not considered by
 *   other widgets.
 */
bool 
S9sTopUi::processButton(
        uint button, 
        uint x, 
        uint y)
{

    if (y == 1)
    {
        if (x >= 19 && x <= 21)
        {
            // The reload/abort button.
            if (m_communicating)
            {
                m_communicating = false;
            } else {
                m_reloadRequested = true;
            }

            return true;
        }
    } else if ((int) y == height())
    {
        if (x >= 2 && x <= 12)
        {
            m_sortOrder = CpuUsage;
            return true;
        } else if (x >= 14 && x <= 27)
        {
            m_sortOrder = MemUsage;
            return true;
        } else if (x >= 29 && x <= 34)
        {
            exit(0);
        }
    }
    
    return S9sDisplay::processButton(button, x, y);
}

bool
S9sTopUi::refreshScreen()
{
    startScreen();
    printHeader();
    
    if (m_nReplies == 0)
        printMiddle("*** Waiting for data. ***");
    
    printFooter();
    return true;
}

void
S9sTopUi::printHeader()
{
    S9sDateTime dt = S9sDateTime::currentDateTime();
    S9sString   title;

    if (!m_clusterName.empty())
    {
        title.sprintf("%s (s9s top)", STR(m_clusterName));
        printf("%s%s%s", "\033]0;", STR(title), "\007");
    }

    title = "S9S TOP";
    ::printf("%s%s%s ", TERM_SCREEN_TITLE_BOLD, STR(title), TERM_SCREEN_TITLE);
    ::printf("%c ", rotatingCharacter());
    ::printf("%s ", STR(dt.toString(S9sDateTime::LongTimeFormat)));

    // Printing the network activity character.
    if (m_communicating || m_reloadRequested)
        ::printf("❌ ");
    else
        ::printf("⟳ ");

    if (m_nReplies > 0)
    {
        ::printf("%s - ", STR(m_clusterName));
        ::printf("%s ", STR(m_clustersReply.clusterStatusText(m_clusterId)));

    } else {
        ::printf("            ");
    }
   
    // If we are in debug mode we print a few internals that help us in
    // development.
    if (m_viewDebug)
    {
        ::printf("0x%02x ",      lastKeyCode());
        ::printf("%02dx%02d ",   width(), height());
        ::printf("%02d:%03d,%03d ", m_lastButton, m_lastX, m_lastY);
    }
        
    printNewLine();
    

    if (m_nReplies > 0)
    {
        switch (m_viewMode)
        {
            case OsProcesses:
                m_cpuStatsReply.printCpuStatLine1();
                printNewLine();

                m_memoryStatsReply.printMemoryStatLine1();
                printNewLine();

                m_memoryStatsReply.printMemoryStatLine2();
                printNewLine();
       
                printProcesses(height() - 6);
                break;

            case SqlProcesses:
                printSqlProcesses(height() - 6);
                break;
        }
    }
}

void 
S9sTopUi::printSqlProcesses(
        int maxLines)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sFormat       pidFormat;
    S9sFormat       commandFormat;
    S9sFormat       timeFormat;
    S9sFormat       userFormat;
    S9sFormat       hostNameFormat;
    S9sFormat       instanceFormat;
    int             nLines;

    sort(m_sqlProcesses.begin(), m_sqlProcesses.end(), 
            S9sSqlProcess::compareSqlProcess);

    nLines = 0;
    for (uint idx = 0u; idx < m_sqlProcesses.size(); ++idx)
    {
        S9sSqlProcess &process = m_sqlProcesses[idx];
        int            pid = process.pid();
        S9sString      command = process.command();
        int            time = process.time();
        S9sString      user = process.userName();
        S9sString      hostName = process.hostName();
        S9sString      instance = process.instance();
        
        if (!options->isStringMatchExtraArguments(command))
            continue;

        pidFormat.widen(pid);
        userFormat.widen(user);
        hostNameFormat.widen(hostName);
        instanceFormat.widen(instance);
        commandFormat.widen(command);
        timeFormat.widen(time);

        if (maxLines > 0 && nLines + 1 >= maxLines)
            break;

        ++nLines;
    }

    nLines = 0;
    for (uint idx = 0u; idx < m_sqlProcesses.size(); ++idx)
    {
        S9sSqlProcess &process = m_sqlProcesses[idx];
        int            pid = process.pid();
        S9sString      user = process.userName();
        S9sString      query = process.query("");
        S9sString      hostName = process.hostName();
        S9sString      instance = process.instance();
        S9sString      command = process.command();
        int            time = process.time();

        if (!options->isStringMatchExtraArguments(command))
            continue;

        query.replace("(\n", "(");
        query.replace("\n ", " ");
        query.replace("\n", "\\n");

        pidFormat.printf(pid);
        commandFormat.printf(command);
        timeFormat.printf(time);

        ::printf("%s", XTERM_COLOR_ORANGE);
        userFormat.printf(user);
        ::printf("%s", TERM_NORMAL);


        ::printf("%s", XTERM_COLOR_GREEN);
        hostNameFormat.printf(hostName);
        ::printf("%s", TERM_NORMAL);

        instanceFormat.printf(instance);

        if (!query.empty())
        {
            ::printf("%s",  XTERM_COLOR_SQL);
            ::printf("%s ", STR(query));
            ::printf("%s",  TERM_NORMAL);
        } else {
            ::printf("- ");
        }

        printNewLine();
        
        if (maxLines > 0 && nLines + 1 >= maxLines)
            break;

        ++nLines;
    }
   
    for (int n = nLines; n + 1 < maxLines; ++n)
        printNewLine();

}

static bool 
compareProcessPid(
        const S9sProcess &a,
        const S9sProcess &b)
{
    return a.pid() < b.pid();
}

static bool 
compareProcessCpu(
        const S9sProcess &a,
        const S9sProcess &b)
{
    if (a.cpuUsage() == b.cpuUsage())
        return a.pid() > b.pid();

    return a.cpuUsage() > b.cpuUsage();
}

static bool 
compareProcessMem(
        const S9sProcess &a,
        const S9sProcess &b)
{
    if (a.memUsage() == b.memUsage())
        return a.pid() > b.pid();

    return a.memUsage() > b.memUsage();
}

/**
 * \param maxLines The number of lines we have on the screen for the printout.
 *
 * This is where we print the OS process list that was prepared by the
 * getProcesses() method.
 */
void
S9sTopUi::printProcesses(
        int maxLines)
{
    S9sOptions     *options = S9sOptions::instance();
    S9sFormat       pidFormat;
    S9sFormat       userFormat(userColorBegin(), userColorEnd());
    S9sFormat       hostFormat(XTERM_COLOR_GREEN, TERM_NORMAL);
    S9sFormat       priorityFormat;
    S9sFormat       virtFormat;
    S9sFormat       resFormat;
    S9sFormat       stateFormat;
    S9sFormat       cpuFormat;
    S9sFormat       memFormat;
    S9sFormat       commandFormat("\033[1;2m\033[38;5;46m", TERM_NORMAL);
    int             nLines;

    switch (m_sortOrder)
    {
        case PidOrder:
            sort(m_processes.begin(), m_processes.end(), compareProcessPid);
            break;

        case CpuUsage:
            sort(m_processes.begin(), m_processes.end(), compareProcessCpu);
            break;

        case MemUsage:
            sort(m_processes.begin(), m_processes.end(), compareProcessMem);
            break;
    }
    
    /*
     * Collecting data.
     */
    nLines = 0;
    for (uint idx = 0u; idx < m_processes.size(); ++idx)
    {
        const S9sProcess  &process = m_processes[idx];
        int           pid        = process.pid();
        S9sString     user       = process.userName();
        S9sString     hostName   = process.hostName();
        int           priority   = process.priority();
        S9sString     rss        = process.resMem("");
        S9sString     virtMem    = process.virtMem("");
        S9sString     cpuUsage   = process.cpuUsage("");
        S9sString     memUsage   = process.memUsage("");
        S9sString     executable = process.executable();

        if (!options->isStringMatchExtraArguments(executable))
            continue;

        pidFormat.widen(pid);
        userFormat.widen(user);
        hostFormat.widen(hostName);
        priorityFormat.widen(priority);
        virtFormat.widen(virtMem);
        resFormat.widen(rss);
        cpuFormat.widen(cpuUsage);
        memFormat.widen(memUsage);
        commandFormat.widen(executable);

        if (maxLines > 0 && nLines + 1 >= maxLines)
            break;

        ++nLines;
    }

    // Flickering of the widths is a bit annyoying, so we introduce some minimal
    // widths.
    userFormat.widen("xxxxxxxxxx");
    
    virtFormat.setRightJustify();
    resFormat.setRightJustify();
    cpuFormat.setRightJustify();
    memFormat.setRightJustify();
    
    if (!m_processes.empty())
    {
        pidFormat.widen("PID");
        userFormat.widen("USER");
        hostFormat.widen("HOST");
        priorityFormat.widen("PR");
        virtFormat.widen("VIRT");
        resFormat.widen("RES");
        stateFormat.widen("S");
        cpuFormat.widen("%CPU");
        memFormat.widen("%MEM");
        commandFormat.widen("COMMAND");

        printf("%s", TERM_SCREEN_HEADER);
        pidFormat.printf("PID", false);
        userFormat.printf("USER", false);
        hostFormat.printf("HOST", false);
        priorityFormat.printf("PR", false);
        virtFormat.printf("VIRT", false);
        resFormat.printf("RES", false);
        stateFormat.printf("S", false);
        cpuFormat.printf("%CPU", false);
        memFormat.printf("%MEM", false);
        commandFormat.printf("COMMAND", false);
        printNewLine();
    }
    
    nLines = 0;
    for (uint idx = 0u; idx < m_processes.size(); ++idx)
    {
        const S9sProcess  &process = m_processes[idx];
        int           pid        = process.pid();
        S9sString     user       = process.userName();
        S9sString     hostName   = process.hostName();
        int           priority   = process.priority();

        S9sString     cpuUsage   = process.cpuUsage("");
        S9sString     memUsage   = process.memUsage("");
        S9sString     state      = process.state();

        S9sString     rss        = process.resMem("");
        S9sString     virtMem    = process.virtMem("");
        S9sString     executable = process.executable();
        
        if (!options->isStringMatchExtraArguments(executable))
            continue;

        pidFormat.printf(pid);
        userFormat.printf(user);
        hostFormat.printf(hostName);
        priorityFormat.printf(priority);

        virtFormat.printf(virtMem);
        resFormat.printf(rss);

        printf("%1s ", STR(state));
        cpuFormat.printf(cpuUsage);
        memFormat.printf(memUsage);
        commandFormat.printf(executable);

        printNewLine();
        if (maxLines > 0 && nLines + 1 >= maxLines)
            break;

        ++nLines;
    }
}

void
S9sTopUi::printFooter()
{
    const char *bold   = TERM_SCREEN_TITLE_BOLD;
    const char *normal = TERM_SCREEN_TITLE;

    // Goint to the last line.
    for (;m_lineCounter < height() - 1; ++m_lineCounter)
    {
        ::printf("\n\r");
        ::printf("%s", TERM_ERASE_EOL);
    } 

    ::printf("%s ", normal);
    ::printf("%sC%s-CPU Order ", bold, normal);
    ::printf("%sM%s-Memory Order ", bold, normal);
    ::printf("%sQ%s-Quit ", bold, normal);

    // No new-line at the end, this is the last line.
    ::printf("%s", TERM_ERASE_EOL);
    ::printf("%s", TERM_NORMAL);
    fflush(stdout);
}

void
S9sTopUi::executeTop()
{
    S9sOptions  *options = S9sOptions::instance();
    int          clusterId = options->clusterId();
    int          updateFreq = options->updateFreq();
    bool         success;
    time_t       startTime;

    if (clusterId <= 0)
    {
        PRINT_ERROR("The cluster ID is invalid while executing 'top'.");
        exit(1);
    }

    for (;;)
    {
        startTime = time(NULL);

        switch  (m_viewMode)
        {
            case OsProcesses:
                success = getProcesses();
                break;

            case SqlProcesses:
                success = getSqlProcesses();
                break;
        }

        if (!success)
            break;
        
        while (time(NULL) - startTime < updateFreq && !m_reloadRequested)
            usleep(100000);
    }
}

/**
 * \returns True if everything went well, false on communication error.
 *
 * This method will get the OS processes from the controller. In fact this
 * method will send multiple requests and will get more than the list of
 * processes, but the important part is that this is what we use to refresh the
 * data from the controller when showing the OS processes.
 */
bool
S9sTopUi::getProcesses()
{
    S9sMutexLocker         locker(m_networkMutex);
    S9sOptions            *options     = S9sOptions::instance();
    S9sVariantList         hostList;
    S9sString              clusterStatusText;
    S9sRpcReply            reply;
    S9sRpcReply            clustersReply;
    time_t                 clustersReplyReceived;
    S9sRpcReply            cpuStatsReply;
    S9sRpcReply            memoryStatsReply;
    S9sRpcReply            processReply;
    S9sVector<S9sProcess>  processes;
    int                    clusterId;
    S9sString              clusterName;
    bool                   success = true;

    m_communicating   = true;
    m_reloadRequested = false;

    /*
     * The cluster information.
     */
    clustersReplyReceived = m_clustersReplyReceived;
    clusterId   = options->clusterId();
    clusterName = options->clusterName();

    if (time(NULL) - clustersReplyReceived > 30)
    {
        success = m_client.getCluster(clusterName, clusterId);

        // If the user aborted download.
        if (!m_communicating)
            return true;

        clustersReply = m_client.reply();
        if (!success)
            return success;

        clustersReplyReceived = time(NULL);
    } else {
        clustersReply = m_clustersReply;
    }

    /*
     * The CPU statistics.
     */
    m_client.getCpuStats(clusterId);
    
    // If the user aborted download.
    if (!m_communicating)
        return true;

    cpuStatsReply = m_client.reply();
    
    /*
     * The memory statistics.
     */
    m_client.getMemoryStats(clusterId);

    // If the user aborted download.
    if (!m_communicating)
        return true;

    memoryStatsReply = m_client.reply();
    
    /*
     * Getting the list of the running processes.
     */
    m_client.getRunningProcesses();
    
    // If the user aborted download.
    if (!m_communicating)
        return true;

    processReply = m_client.reply();
    hostList = processReply["data"].toVariantList();
    for (uint idx = 0u; idx < hostList.size(); ++idx)
    {
        S9sString hostName = hostList[idx]["hostname"].toString();
        S9sVariantList processList = hostList[idx]["processes"].toVariantList();
    
        for (uint idx1 = 0u; idx1 < processList.size(); ++idx1)
        {
            S9sVariantMap processMap = processList[idx1].toVariantMap();
            S9sProcess    process;

            processMap["hostname"] = hostName;
            process = processMap;
            processes << process;
        }

        // If the user aborted download.
        if (!m_communicating)
            return true;
    }

    /*
     * Pushing the received data into the object so that the screen refresh can
     * print them.
     */
    m_mutex.lock(); 

    m_clustersReply         = clustersReply;
    m_clustersReplyReceived = clustersReplyReceived;
    m_cpuStatsReply         = cpuStatsReply;
    m_memoryStatsReply      = memoryStatsReply;
    m_processReply          = processReply;
    m_processes             = processes;
    m_clusterId             = clusterId;
    m_clusterName           = m_clustersReply.clusterName(m_clusterId);

    m_communicating         = false;
    m_nReplies++;
    m_refreshCounter++;

    m_mutex.unlock();

    return true;
}

bool
S9sTopUi::getSqlProcesses()
{
    S9sMutexLocker         locker(m_networkMutex);
    //S9sOptions            *options     = S9sOptions::instance();
    S9sRpcReply            getSqlProcessesReply;
    S9sVariantList         variantList;
    S9sVector<S9sSqlProcess> processList;

    m_communicating   = true;
    m_reloadRequested = false;
    
    m_client.getSqlProcesses();
    getSqlProcessesReply = m_client.reply();

    variantList = getSqlProcessesReply["processes"].toVariantList();
    for (size_t idx = 0; idx < variantList.size(); ++idx)
    {
        S9sSqlProcess process = variantList[idx].toVariantMap();

        processList << process;
    }

    m_mutex.lock();     
    m_sqlProcesses = processList;
    m_nReplies++;
    m_refreshCounter++;
    m_mutex.unlock();
    
    m_communicating   = false;
    return true;
}
