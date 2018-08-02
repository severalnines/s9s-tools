/*
 * Severalnines Tools
 * Copyright (C) 2016-2018 Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * s9s-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with s9s-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9sdisplay.h"

#include "S9sOptions"
#include "S9sCluster"
#include "S9sRpcReply"
#include "S9sMutexLocker"
#include "S9sDateTime"

#define DEBUG
//#define WARNING
#include "s9sdebug.h"

#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>

struct termios orig_termios1;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios1);
    ::printf("%s", TERM_CURSOR_ON);
    ::printf("%s", TERM_AUTOWRAP_ON);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios1);
    memcpy(&new_termios, &orig_termios1, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
    
    ::printf("%s", TERM_CURSOR_OFF);
    ::printf("%s", TERM_AUTOWRAP_OFF);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(0, &fds);

    return select(1, &fds, NULL, NULL, &tv);
}


S9sDisplay::S9sDisplay(
        S9sDisplay::DisplayMode mode) :
    m_displayMode(mode),
    m_refreshCounter(0),
    m_lastKey1(0),
    m_columns(0),
    m_rows(0)
{
    set_conio_terminal_mode();
}

S9sDisplay::~S9sDisplay()
{
    reset_terminal_mode();
}

int 
S9sDisplay::exec()
{
    do {
        usleep(500000);
    
        if (kbhit())
        {
            m_lastKey1 = getchar();
#if 0
            #if 0
            usleep(50000);

            while (kbhit())
            {
                m_lastKey1 *= 256;
                m_lastKey1 += getchar();
                usleep(50000);
            }
            #else
            if (m_lastKey1 == 27 && kbhit())
            {
                m_lastKey1 *= 256;
                m_lastKey1 += getchar();
                m_lastKey1 *= 256;
                m_lastKey1 += getchar();
            }
            #endif
#endif
            m_mutex.lock();
            processKey(m_lastKey1);
            m_mutex.unlock();
        }

        m_mutex.lock();
        refreshScreen();
        m_mutex.unlock();
    } while (!shouldStop());

    return 0;
}

void
S9sDisplay::processKey(
        int key)
{
    switch (key)
    {
        case 'q':
        case 'Q':
        case 0x1b:
        case 3:
            ::printf("%s", TERM_CLEAR_SCREEN);
            ::printf("Exiting on key press.\n");
            fflush(stdout);
            exit(0);
            break;

        case 'c':
        case 'C':
            m_displayMode = WatchClusters;
            ::printf("%s", TERM_CLEAR_SCREEN);
            break;
        
        case 'n':
        case 'N':
            m_displayMode = WatchNodes;
            ::printf("%s", TERM_CLEAR_SCREEN);
            break;
        
        case 'j':
        case 'J':
            m_displayMode = WatchJobs;
            ::printf("%s", TERM_CLEAR_SCREEN);
            break;
    }
}

/**
 * The mutex is locked when this private method is called.
 */
void
S9sDisplay::refreshScreen()
{
    switch (m_displayMode)
    {
        case WatchNodes:
            printNodes();
            break;

        case WatchClusters:
            printClusters();
            break;
        
        case WatchJobs:
            printJobs();
            break;
            
        case PrintEvents:
            break;
    }
}

void 
S9sDisplay::eventCallback(
        S9sEvent &event)
{
    S9sMutexLocker    locker(m_mutex);
    S9sOptions       *options = S9sOptions::instance();

    switch (m_displayMode)
    {
        case PrintEvents:
            // Filtration by event class and subclass (event-name).
            if (!options->eventTypeEnabled(event.eventTypeString()))
                return;
    
            if (!options->eventNameEnabled(event.eventName()))
                return;

            break;

        case WatchNodes:
        case WatchClusters:
        case WatchJobs:
            //if (event.eventTypeString() != "EventHost")
            //    return;
            break;
    }

    // optional filtration by clusterId
    if (options->clusterId() > S9S_INVALID_CLUSTER_ID
        && options->clusterId() != event.clusterId())
    {
        return;
    }

    processEvent(event);
}


void 
S9sDisplay::processEvent(
        S9sEvent &event)
{
    ++m_refreshCounter;

    if (event.hasCluster())
    {
        S9sCluster cluster = event.cluster();
        // FIXME: what about cluster delete events?
        if (cluster.clusterId() != 0)
            m_clusters[cluster.clusterId()] = cluster;
    }

    if (event.hasJob())
    {
        S9sJob job = event.job();
            
        m_jobs[job.id()] = job;
    }
    
    if (event.hasHost())
    {
        S9sNode  node;
        bool     found = false;

        node = event.host();
        for (uint idx = 0u; idx < m_nodes.size(); ++idx)
        {
            if (m_nodes[idx].hostName() != node.hostName())
                continue;
        
            if (m_nodes[idx].port() != node.port())
                continue;

            if (m_nodes[idx].id() != node.id())
                continue;

            m_nodes[idx] = node;
            found = true;
            break;
        }

        if (!found)
            m_nodes << node;
    }

    switch (m_displayMode)
    {
        case PrintEvents:
            processEventList(event);
            break;

        case WatchNodes:
            printNodes();
            ++m_refreshCounter;
            break;

        case WatchClusters:
            printClusters();
            ++m_refreshCounter;
            break;
        
        case WatchJobs:
            printJobs();
            ++m_refreshCounter;
            break;
    }
}

void 
S9sDisplay::processEventList(
        S9sEvent &event)
{
    S9sOptions       *options = S9sOptions::instance();
    S9sString         output;

    if (options->isJsonRequested())
    {
        output = event.toVariantMap().toString();
    } else {
        output = event.toOneLiner();
    }

    output.replace("\n", "\n\r");
    if (!output.empty())
        ::printf("%s\n\r", STR(output));
}

char
S9sDisplay::rotatingCharacter() const
{
    char charset[] = { '/', '-', '\\',  '|' };

    return charset[m_refreshCounter % 3];
}

void
S9sDisplay::startScreen()
{
    struct winsize w;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    if (m_refreshCounter == 0 || 
            m_columns == w.ws_col ||
            m_rows    == w.ws_row)
    {
        ::printf("%s", TERM_CLEAR_SCREEN);
        ::printf("%s", TERM_HOME);
    } else {
        ::printf("%s", TERM_HOME);
    }
    
    m_columns = w.ws_col;
    m_rows    = w.ws_row;
    m_lineCounter = 0;
}

void
S9sDisplay::printHeader()
{
    S9sDateTime dt = S9sDateTime::currentDateTime();
    S9sString   title;

    switch (m_displayMode)
    {
        case WatchNodes:
            title = "S9S NODE VIEW      ";
            break;

        case WatchClusters:
            title = "S9S CLUSTER VIEW   ";
            break;

        case WatchJobs:
            title = "S9S JOB VIEW       ";
            break;
            
        default:
            break;
    }

    ::printf("%s", TERM_SCREEN_TITLE);
    ::printf("%s ", STR(title));
    ::printf("%c ", rotatingCharacter());
    ::printf("%s ", STR(dt.toString(S9sDateTime::LongTimeFormat)));
    ::printf("%lu node(s) ", m_nodes.size());
    ::printf("%lu cluster(s) ", m_clusters.size());
    ::printf("%lu jobs(s) ", m_jobs.size());
    ::printf("0x%02x ", m_lastKey1);
    ::printf("%02dx%02d ", m_columns, m_rows);

    ::printf("%s", TERM_ERASE_EOL);
    ::printf("\r\n");
    ::printf("%s", TERM_NORMAL);
    m_lineCounter++;
}

void
S9sDisplay::printFooter()
{
    ::printf("%s", TERM_ERASE_EOL);
    for (;m_lineCounter < m_rows - 1; ++m_lineCounter)
    {
        ::printf("\n\r");
        ::printf("%s", TERM_ERASE_EOL);
    }
    
    const char *normal = "\033[0m\033[2m\033[48;5;20m";
    ::printf("%s ", TERM_SCREEN_TITLE);

    ::printf("%sn%s-nodes  ", "\033[1m", normal);
    ::printf("%sc%s-clusters  ", "\033[1m", normal);
    ::printf("%sj%s-jobs  ", "\033[1m", normal);

    ::printf("%s", TERM_ERASE_EOL);
    ::printf("%s", TERM_NORMAL);
    fflush(stdout);
    m_lineCounter++;
}

void
S9sDisplay::printMiddle(
        const S9sString text)
{
    int nSpaces;

    for (;m_lineCounter < m_rows / 2;)
    {
        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\r\n");
        ++m_lineCounter;
    }

    nSpaces = (m_columns - text.length()) / 2;
    for (;nSpaces > 0;--nSpaces)
        ::printf(" ");

    ::printf("%s", STR(text));
    ::printf("%s", TERM_ERASE_EOL);
    ::printf("\r\n");
    ++m_lineCounter;
}

void
S9sDisplay::printClusters()
{
    S9sFormat versionFormat;
    S9sFormat idFormat;
    S9sFormat typeFormat;
    S9sFormat stateFormat;
    S9sFormat nameFormat;
    S9sFormat messageFormat;

    startScreen();
    printHeader();

    /*
     * Collecting some information.
     */
    foreach (const S9sCluster &cluster, m_clusters)
    {
        versionFormat.widen(cluster.vendorAndVersion());
        idFormat.widen(cluster.clusterId());
        typeFormat.widen(cluster.clusterType());
        stateFormat.widen(cluster.state());
        nameFormat.widen(cluster.name());
        messageFormat.widen(cluster.statusText());
    }

    if (!m_clusters.empty())
    {
        versionFormat.widen("VERSION");
        idFormat.widen("ID");
        stateFormat.widen("STATE");
        typeFormat.widen("TYPE");
        nameFormat.widen("CLUSTER STATE");
        messageFormat.widen("MESSAGE");

        printf("%s", TERM_SCREEN_HEADER /*m_formatter.headerColorBegin()*/);
        versionFormat.printf("VERSION");
        idFormat.printf("ID");
        stateFormat.printf("STATE");
        typeFormat.printf("TYPE");
        nameFormat.printf("CLUSTER STATE");
        messageFormat.printf("MESSAGE");
        
        printf("%s%s\n\r", TERM_ERASE_EOL, m_formatter.headerColorEnd());
        ++m_lineCounter;
    } else {
        printMiddle("*** No clusters. ***");
    }

    /*
     * Printing.
     */
    foreach (const S9sCluster &cluster, m_clusters)
    {
        versionFormat.printf(cluster.vendorAndVersion());
        idFormat.printf(cluster.clusterId());
        
        printf("%s", m_formatter.clusterStateColorBegin(cluster.state()));
        stateFormat.printf(cluster.state());
        printf("%s", m_formatter.clusterStateColorEnd());

        typeFormat.printf(cluster.clusterType());

        printf("%s", m_formatter.clusterColorBegin());
        nameFormat.printf(cluster.name());
        printf("%s", m_formatter.clusterColorEnd());
        
        messageFormat.printf(cluster.statusText());

        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\n\r");
        ++m_lineCounter;
    }

    printFooter();
}

void
S9sDisplay::printJobs()
{
    S9sFormat idFormat;
    S9sFormat stateFormat;
    S9sFormat progressFormat;
    S9sFormat titleFormat;
    S9sFormat statusTextFormat;

    startScreen();
    printHeader();
    
    foreach (const S9sJob &job, m_jobs)
    {
        S9sString statusText = S9sString::html2ansi(job.statusText());

        idFormat.widen(job.id());
        stateFormat.widen(job.status());
        progressFormat.widen("[--------- ]");
        titleFormat.widen(job.title());
        statusTextFormat.widen(statusText);
    }

    if (!m_jobs.empty())
    {
        idFormat.widen("ID");
        stateFormat.widen("STATE");
        progressFormat.widen("PROGRESS");
        titleFormat.widen("TITLE");
        titleFormat.widen("STATUS");

        printf("%s", TERM_SCREEN_HEADER /*m_formatter.headerColorBegin()*/);
        idFormat.printf("ID");
        stateFormat.printf("STATE");
        progressFormat.printf("PROGRESS");
        titleFormat.printf("TITLE");
        statusTextFormat.printf("STATUS");
        printf("%s%s\n\r", TERM_ERASE_EOL, m_formatter.headerColorEnd());
        ++m_lineCounter;
    } else {
        printMiddle("*** No jobs. ***");
    }

    foreach (const S9sJob &job, m_jobs)
    {
        S9sString statusText  = S9sString::html2ansi(job.statusText());
        bool      hasPercent  = job.hasProgressPercent();
        S9sString status      = job.status();
        double    percent     = job.progressPercent();
        S9sString progressBar;

        if (status == "FINISHED")
        {
            percent = 100.0;
            hasPercent = true;
            progressBar = S9sRpcReply::progressBar(percent, true);
            progressBar = "             ";
        } else if (status == "FAILED")
        {
            progressBar = "             ";
        } else if (status == "CREATED")
        {
            progressBar = "             ";
        } else if (status == "SCHEDULED")
        {
            progressBar = "             ";
        } else if (status == "DEFINED")
        {
            progressBar = "             ";
        } else {
            if (hasPercent)
                progressBar = S9sRpcReply::progressBar(percent, true);
            else
                progressBar = S9sRpcReply::progressBar(true);
        }

        idFormat.printf(job.id());
        
        printf("%s", m_formatter.jobStateColorBegin(job.status()));
        stateFormat.printf(job.status());
        printf("%s", m_formatter.jobStateColorEnd());

        ::printf("%s", STR(progressBar));

        ::printf("%s", TERM_BOLD);
        titleFormat.printf(job.title());
        ::printf("%s", TERM_NORMAL);

        statusTextFormat.printf(statusText);

        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\n\r");
        ++m_lineCounter;
    }

    printFooter();
}

void
S9sDisplay::printNodes()
{
    S9sFormat   versionFormat;
    S9sFormat   clusterNameFormat;
    S9sFormat   clusterIdFormat;
    S9sFormat   hostNameFormat;
    S9sFormat   portFormat;
    const char *beginColor, *endColor;

    startScreen();
    printHeader();

    /*
     * Collecting information for formatting.
     */
    for (uint idx = 0u; idx < m_nodes.size(); ++idx)
    {
        const S9sNode &node = m_nodes[idx];
        S9sString      clusterName = "-";

        if (m_clusters.contains(node.clusterId()))
            clusterName = m_clusters[node.clusterId()].name();

        versionFormat.widen(node.version());
        clusterIdFormat.widen(node.clusterId());
        clusterNameFormat.widen(clusterName);
        hostNameFormat.widen(node.hostName());
        portFormat.widen(node.port());
    }

    /*
     * Printing the header.
     */
    if (!m_nodes.empty())
    {
        versionFormat.widen("VERSION");
        clusterIdFormat.widen("CID");
        clusterNameFormat.widen("CLUSTER");
        hostNameFormat.widen("HOST");
        portFormat.widen("PORT");

        printf("%s", TERM_SCREEN_HEADER /*m_formatter.headerColorBegin()*/);
        printf("STAT ");
        versionFormat.printf("VERSION");
        clusterIdFormat.printf("CID");
        clusterNameFormat.printf("CLUSTER");
        hostNameFormat.printf("HOST");
        portFormat.printf("PORT");
        printf("COMMENT");
        printf("%s%s\n\r", TERM_ERASE_EOL, m_formatter.headerColorEnd());
        ++m_lineCounter;
    } else {
        printMiddle("*** No nodes. ***");
    }

    for (uint idx = 0u; idx < m_nodes.size(); ++idx)
    {
        const S9sNode &node = m_nodes[idx];
        S9sString      clusterName = "-";

        if (m_clusters.contains(node.clusterId()))
            clusterName = m_clusters[node.clusterId()].name();

        beginColor = S9sRpcReply::hostStateColorBegin(node.hostStatus());
        endColor   = S9sRpcReply::hostStateColorEnd();
        hostNameFormat.setColor(beginColor, endColor);

        ::printf("%c", node.nodeTypeFlag());
        ::printf("%c", node.hostStatusFlag());
        ::printf("%c", node.roleFlag());
        ::printf("%c ", node.maintenanceFlag());
        versionFormat.printf(node.version());
        clusterIdFormat.printf(node.clusterId());

        printf("%s", m_formatter.clusterColorBegin());
        clusterNameFormat.printf(clusterName);
        printf("%s", m_formatter.clusterColorEnd());

        hostNameFormat.printf(node.hostName());
        portFormat.printf(node.port());

        ::printf("%s ", STR(node.message()));
        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\n\r");
        ++m_lineCounter;
    }

    printFooter();
}

void
S9sDisplay::eventHandler(
        const S9sVariantMap &jsonMessage,
        void                *userData)
{
    if (userData == NULL)
    {
        S9S_WARNING("userData is NULL.");
        return;
    }

    if (!jsonMessage.contains("class_name") ||
            jsonMessage.at("class_name").toString() != "CmonEvent")
    {
        return;
    }

    S9sEvent event = jsonMessage;
    S9sDisplay *display = (S9sDisplay *) userData;

    display->eventCallback(event);
}
