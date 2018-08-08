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
#include "s9smonitor.h"

#include "S9sOptions"
#include "S9sCluster"
#include "S9sContainer"
#include "S9sRpcReply"
#include "S9sMutexLocker"
#include "S9sDateTime"

#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sMonitor::S9sMonitor(
        S9sMonitor::DisplayMode mode) : 
    S9sDisplay(),
    m_displayMode(mode),
    m_viewDebug(false),
    m_viewObjects(false),
    m_selectionIndex(0),
    m_selectionEnabled(true)
{
}

S9sMonitor::~S9sMonitor()
{
}

/**
 * \returns How many containers found.
 */
int
S9sMonitor::nContainers() const
{
    int retval = 0;

    foreach (const S9sServer &server, m_servers)
        retval += server.nContainers();

    return retval;
}

/**
 * The mutex is locked when this private method is called.
 */
void
S9sMonitor::refreshScreen()
{
    switch (m_displayMode)
    {
        case WatchNodes:
            printNodes();
            break;
        
        case WatchContainers:
            printContainers();
            break;

        case WatchClusters:
            printClusters();
            break;
        
        case WatchJobs:
            printJobs();
            break;
    
        case WatchEvents:
            printEvents();
            break;

        case PrintEvents:
            break;
    }
}

/**
 * Print method for the "containers" view.
 */
void
S9sMonitor::printContainers()
{
    S9sFormat  typeFormat;
    S9sFormat  templateFormat;
    S9sFormat  stateFormat;
    S9sFormat  ipFormat;
    S9sFormat  serverFormat;
    S9sFormat  aliasFormat;

    startScreen();
    printHeader();
    
    foreach (const S9sServer &server, m_servers)
    {
        S9sVariantList maps = server.containers();

        for (uint idx = 0u; idx < maps.size(); ++idx)
        {
            S9sContainer container = maps[idx].toVariantMap();
            S9sString    ipAddress = container.ipAddress(
                    S9s::AnyIpv4Address, "-");

            typeFormat.widen(container.type());
            templateFormat.widen(container.templateName("-"));
            stateFormat.widen(container.state());
            ipFormat.widen(ipAddress);
            serverFormat.widen(container.parentServerName());
            aliasFormat.widen(container.alias());
        }
    }
    
    if (nContainers() > 0)
    {
        typeFormat.widen("TYPE");
        templateFormat.widen("TEMPLATE");
        stateFormat.widen("STATE");
        ipFormat.widen("IP ADDRESS");
        serverFormat.widen("SERVER");
        aliasFormat.widen("NAME");
        
        printf("%s", TERM_SCREEN_HEADER);
        typeFormat.printf("TYPE");
        templateFormat.printf("TEMPLATE");
        stateFormat.printf("STATE");
        ipFormat.printf("IP ADDRESS");
        serverFormat.printf("SERVER");
        aliasFormat.printf("NAME");

        printf("%s%s\n\r", TERM_ERASE_EOL, m_formatter.headerColorEnd());
        ++m_lineCounter; 
    } else {
        printMiddle("*** No containers. ***");
    }

    foreach (const S9sServer &server, m_servers)
    {
        S9sVariantList maps = server.containers();

        for (uint idx = 0u; idx < maps.size(); ++idx)
        {
            S9sContainer container = maps[idx].toVariantMap();
            S9sString    ipAddress = container.ipAddress(
                    S9s::AnyIpv4Address, "-");
            int          stateAsChar = container.stateAsChar();
            bool         selected = 
                (int) idx == m_selectionIndex &&
                m_selectionEnabled;

            if (!selected)
            {
                typeFormat.printf(STR(container.type()));
                templateFormat.printf(container.templateName("-"));
                stateFormat.printf(STR(container.state()));

                ::printf("%s", m_formatter.ipColorBegin(ipAddress));
                ipFormat.printf(STR(ipAddress));
                ::printf("%s", m_formatter.ipColorEnd(ipAddress));

                ::printf("%s", m_formatter.serverColorBegin());
                serverFormat.printf(container.parentServerName());
                ::printf("%s", m_formatter.serverColorEnd());

                ::printf("%s", m_formatter.containerColorBegin(stateAsChar));
                aliasFormat.printf(container.alias());
                ::printf("%s", m_formatter.containerColorEnd());
            
                ::printf("%s", TERM_ERASE_EOL);
            } else {
                ::printf("%s", "\033[1m\033[48;5;4m");
                typeFormat.printf(STR(container.type()));
                templateFormat.printf(container.templateName("-"));
                stateFormat.printf(STR(container.state()));
                ipFormat.printf(STR(ipAddress));
                serverFormat.printf(container.parentServerName());
                aliasFormat.printf(container.alias());
                ::printf("%s", TERM_ERASE_EOL);
                ::printf("%s", TERM_NORMAL);
            }

            ::printf("\n\r");
            ++m_lineCounter;
            
            if (m_lineCounter >= rows() - 1)
                break;
        }
            
    }
    
    printFooter();
}



/**
 * Print method for the "clusters" mode.
 */
void
S9sMonitor::printClusters()
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
        
        if (m_lineCounter >= rows() - 1)
            break;
    }

    printFooter();
}

/**
 * Print method for the "jobs" mode.
 */
void
S9sMonitor::printJobs()
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
        printMiddle("*** No running jobs. ***");
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
        
        if (m_lineCounter >= rows() - 1)
            break;
    }

    printFooter();
}

/**
 * \param event The event that arrived and shall be processed.
 */
void 
S9sMonitor::processEvent(
        S9sEvent &event)
{
    ++m_refreshCounter;

    // The events themselves.
    m_events << event;

    while (m_events.size() > 300)
        m_events.takeFirst();

    // The clusters.
    if (event.hasCluster())
    {
        S9sCluster cluster = event.cluster();
        // FIXME: what about cluster delete events?
        if (cluster.clusterId() != 0)
            m_clusters[cluster.clusterId()] = cluster;
    }

    // The jobs.
    if (event.hasJob())
    {
        S9sJob job = event.job();
            
        m_jobs[job.id()] = job;
        m_jobActivity[job.id()] = time(NULL);
    }
    
    // The hosts.
    if (event.hasHost())
    {
        S9sNode  node;

        node = event.host();
        m_nodes[node.id()] = node;
        m_eventsForNodes[node.id()] = event;
    }
    
    // The servers (together with the containers).
    if (event.hasServer())
    {
        S9sServer  server;

        server = event.server();
        m_servers[server.id()] = server;
    }

    removeOldObjects();

    switch (m_displayMode)
    {
        case PrintEvents:
            processEventList(event);
            break;

        case WatchNodes:
            printNodes();
            ++m_refreshCounter;
            break;
        
        case WatchContainers:
            printContainers();
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
        
        case WatchEvents:
            printEvents();
            ++m_refreshCounter;
            break;
    }
}

/**
 * This is where we process the 
 *   s9s event --list 
 * list. It just prints the events and does nothing else.
 */
void 
S9sMonitor::processEventList(
        S9sEvent &event)
{
    S9sOptions       *options = S9sOptions::instance();
    S9sString         output;

    if (options->isJsonRequested())
    {
        output = event.toVariantMap().toString();
    } else {
        output = event.toOneLiner(options->isDebug());
    }

    output.replace("\n", "\n\r");
    if (!output.empty())
        ::printf("%s\n\r", STR(output));
}

/**
 * This method is called from time to time to remove the old objects e.g. the
 * jobs that are finished a while ago.
 */
void
S9sMonitor::removeOldObjects()
{
    S9sVector<int> jobIds = m_jobs.keys();
    time_t         now    = time(NULL);

    for (uint idx = 0u; idx < jobIds.size(); ++idx)
    {
        int jobId = jobIds[idx];

        if (now - m_jobActivity[jobId] < 10)
            continue;

        if (m_jobs[jobId].status() != "FINISHED" &&
                m_jobs[jobId].status() != "FAILED")
        {
            continue;
        }

        m_jobs.erase(jobId);
        m_jobActivity.erase(jobId);
    }
}

/**
 * Printing the top part of the screen.
 */
void
S9sMonitor::printHeader()
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
        
        case WatchContainers:
            title = "S9S CONTAINER VIEW ";
            break;
        
        case WatchEvents:
            title = "S9S EVENT VIEW     ";
            break;
            
        default:
            break;
    }

    ::printf("%s%s%s ", TERM_SCREEN_TITLE_BOLD, STR(title), TERM_SCREEN_TITLE);
    ::printf("%c ", rotatingCharacter());
    ::printf("%s ", STR(dt.toString(S9sDateTime::LongTimeFormat)));
    ::printf("%lu node(s) ", m_nodes.size());
    ::printf("%d VM(s) ", nContainers());
    ::printf("%lu cluster(s) ", m_clusters.size());
    ::printf("%lu jobs(s) ", m_jobs.size());

    if (m_viewDebug)
    {
        ::printf("0x%02x ",      lastKeyCode());
        ::printf("%02dx%02d ",   columns(), rows());
        ::printf("%02d:%03d,%03d ", m_lastButton, m_lastX, m_lastY);
    }

    ::printf("%s", TERM_ERASE_EOL);
    ::printf("\r\n");
    ::printf("%s", TERM_NORMAL);
    m_lineCounter++;
}

/**
 * Printing the bottom part of the screen.
 */
void
S9sMonitor::printFooter()
{
    ::printf("%s", TERM_ERASE_EOL);
    for (;m_lineCounter < rows() - 1; ++m_lineCounter)
    {
        ::printf("\n\r");
        ::printf("%s", TERM_ERASE_EOL);
    }
    
    const char *bold   = TERM_SCREEN_TITLE_BOLD;
    const char *normal = TERM_SCREEN_TITLE;

    ::printf("%s ", normal);
    ::printf("%sN%s-Nodes ", bold, normal);
    ::printf("%sC%s-Clusters ", bold, normal);
    ::printf("%sJ%s-Jobs ", bold, normal);
    ::printf("%sV%s-Containers ", bold, normal);
    ::printf("%sE%s-Events ", bold, normal);
    ::printf("%sD%s-Debug mode ", bold, normal);
    ::printf("%sQ%s-Quit", bold, normal);
   
    if (!m_outputFileName.empty())
        ::printf("    [%s]", STR(m_outputFileName));

    ::printf("%s", TERM_ERASE_EOL);
    ::printf("%s", TERM_NORMAL);
    fflush(stdout);
    m_lineCounter++;
}




/**
 * Print method for the nodes view.
 */
void
S9sMonitor::printNodes()
{
    S9sFormat   sourceFileFormat(XTERM_COLOR_BLUE, TERM_NORMAL);
    S9sFormat   sourceLineFormat;
    S9sFormat   versionFormat;
    S9sFormat   clusterNameFormat;
    S9sFormat   clusterIdFormat;
    S9sFormat   hostNameFormat;
    S9sFormat   portFormat;

    S9sFormat   aclFormat;
    S9sFormat   ownerFormat(m_formatter.userColorBegin(), m_formatter.userColorEnd());
    S9sFormat   groupFormat(m_formatter.groupColorBegin(), m_formatter.groupColorEnd());
    S9sFormat   pathFormat(m_formatter.ipColorBegin(), m_formatter.ipColorEnd());

    const char *beginColor, *endColor;

    startScreen();
    printHeader();

    /*
     * Collecting information for formatting.
     */
    foreach (const S9sNode &node, m_nodes)
    {
        S9sEvent       event = m_eventsForNodes[node.id()];
        S9sString      clusterName = "-";

        if (m_clusters.contains(node.clusterId()))
            clusterName = m_clusters[node.clusterId()].name();

        sourceFileFormat.widen(event.senderFile());
        sourceLineFormat.widen(event.senderLine());
        versionFormat.widen(node.version());
        clusterIdFormat.widen(node.clusterId());
        clusterNameFormat.widen(clusterName);
        hostNameFormat.widen(node.hostName());
        portFormat.widen(node.port());
            
        aclFormat.widen("n" + node.aclShortString());
        ownerFormat.widen(node.ownerName());
        groupFormat.widen(node.groupOwnerName());
        pathFormat.widen(node.fullCdtPath());
    }

    /*
     * Printing the header.
     */
    if (!m_nodes.empty())
    {
        sourceFileFormat.widen("SOURCE FILE");
        sourceLineFormat.widen("LINE");

        versionFormat.widen("VERSION");
        clusterIdFormat.widen("CID");
        clusterNameFormat.widen("CLUSTER");
        hostNameFormat.widen("HOST");
        portFormat.widen("PORT");

        printf("%s", TERM_SCREEN_HEADER);
       
        if (m_viewDebug)
        {
            sourceFileFormat.printf("SOURCE FILE", false);
            sourceLineFormat.printf("LINE");
        }

        if (m_viewObjects)
        {
            aclFormat.printf("MODE", false);
            ownerFormat.printf("OWNER", false);
            groupFormat.printf("GROUP", false);
            pathFormat.printf("PATH", false);
        } else {
            printf("STAT ");
            versionFormat.printf("VERSION");
            clusterIdFormat.printf("CID");
            clusterNameFormat.printf("CLUSTER");
            hostNameFormat.printf("HOST");
            portFormat.printf("PORT");
            printf("COMMENT");
        }

        printf("%s%s\n\r", TERM_ERASE_EOL, m_formatter.headerColorEnd());
        ++m_lineCounter;
    } else {
        printMiddle("*** No nodes. ***");
    }

    foreach (const S9sNode &node, m_nodes)
    {
        S9sEvent       event = m_eventsForNodes[node.id()];
        S9sString      clusterName = "-";

        if (m_clusters.contains(node.clusterId()))
            clusterName = m_clusters[node.clusterId()].name();

        beginColor = S9sRpcReply::hostStateColorBegin(node.hostStatus());
        endColor   = S9sRpcReply::hostStateColorEnd();
        hostNameFormat.setColor(beginColor, endColor);

        if (m_viewDebug)
        {
            sourceFileFormat.printf(event.senderFile());
            sourceLineFormat.printf(event.senderLine());
        }

        if (m_viewObjects)
        {
            aclFormat.printf("n" + node.aclShortString());
            ownerFormat.printf(node.ownerName());
            groupFormat.printf(node.groupOwnerName());
            pathFormat.printf(node.fullCdtPath());

        } else {
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
        }

        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\n\r");
        ++m_lineCounter;
    }

    printFooter();
}

/**
 * Printing method for the event view.
 */
void
S9sMonitor::printEvents()
{
    int startIndex;

    startScreen();
    printHeader();

    startIndex = m_events.size() - (rows() - 2);
    if (startIndex < 0)
        startIndex = 0;

    for (uint idx = startIndex; idx < m_events.size(); ++idx)
    {
        S9sEvent  &event = m_events[idx];
        S9sString  line;
        
        line = event.toOneLiner(m_viewDebug);

        line.replace("\n", "\\n");
        line.replace("\r", "\\r");
        
        ::printf("%s ", STR(line));
        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\n\r");
        ++m_lineCounter;
    }

    printFooter();
}

void
S9sMonitor::processKey(
        int key)
{
    switch (key)
    {
        case 'q':
        case 'Q':
        case 0x1b:
        case 3:
            //::printf("\n\r");
            //::printf("%s", TERM_CLEAR_SCREEN);
            //::printf("%s", TERM_HOME);
            //::printf("\n\r");
            //::printf("Exiting on key press.\n\r");
            //fflush(stdout);
            exit(0);
            break;

        case 'c':
        case 'C':
            m_displayMode = WatchClusters;
            //::printf("%s", TERM_CLEAR_SCREEN);
            break;
       
        case 'd':
        case 'D':
            // Turning on and off teh debug mode.
            m_viewDebug = !m_viewDebug;
            break;

        case 'o':
        case 'O':
            m_viewObjects = !m_viewObjects;
            break;

        case 'n':
        case 'N':
            m_displayMode = WatchNodes;
            //::printf("%s", TERM_CLEAR_SCREEN);
            break;
        
        case 'j':
        case 'J':
            m_displayMode = WatchJobs;
            //::printf("%s", TERM_CLEAR_SCREEN);
            break;
        
        case 'v':
        case 'V':
            m_displayMode = WatchContainers;
            //::printf("%s", TERM_CLEAR_SCREEN);
            break;
        
        case 'e':
        case 'E':
            m_displayMode = WatchEvents;
            //::printf("%s", TERM_CLEAR_SCREEN);
            break;

        case S9S_KEY_DOWN:
            switch (m_displayMode)
            {
                case PrintEvents:
                    break;

                case WatchEvents:
                case WatchNodes:
                case WatchClusters:
                case WatchJobs:
                    break;

                case WatchContainers:
                    if (m_selectionIndex < nContainers())
                        ++m_selectionIndex;
                    break;
            }
            break;

        case S9S_KEY_UP:
            switch (m_displayMode)
            {
                case PrintEvents:
                    break;

                case WatchEvents:
                case WatchNodes:
                case WatchClusters:
                case WatchJobs:
                    break;

                case WatchContainers:
                    if (m_selectionIndex > 0)
                        --m_selectionIndex;
                    break;
            }
            break;
    }
}

void 
S9sMonitor::processButton(
        uint button, 
        uint x, 
        uint y)
{
    S9sDisplay::processButton(button, x, y);

    if ((int) y == rows())
    {
        if (x >= 2 && x <= 8)
        {
            m_displayMode = WatchNodes;            
        } else if (x >= 10 && x <= 19)
        {
            m_displayMode = WatchClusters;
        } else if (x >= 21 && x <= 26)
        {
            m_displayMode = WatchJobs;
        } else if (x >= 28 && x <= 39)
        {
            m_displayMode = WatchContainers;
        } else if (x >= 41 && x <= 48)
        {
            m_displayMode = WatchEvents;
        } else if (x >= 50 && x <= 61)
        {
            m_viewDebug = !m_viewDebug;
        } else if (x >= 63 && x <= 68)
        {
            exit(0);
        }
    }
}

/**
 * \param event The event to process.
 *
 * This is the function that will be called when an event came to be processed.
 */
void 
S9sMonitor::eventCallback(
        S9sEvent &event)
{
    S9sMutexLocker    locker(m_mutex);
    S9sOptions       *options = S9sOptions::instance();

    
    if (!m_outputFileName.empty())
    {
        bool success;

        success = m_outputFile.fprintf("%s\n\n", STR(event.toString()));
        if (!success)
        {
            PRINT_ERROR("%s", STR(m_outputFile.errorString()));
            exit(1);
        }

        m_outputFile.flush();
    }

    switch (m_displayMode)
    {
        case PrintEvents:
            // Filtration by event class and subclass (event-name).
            if (!options->eventTypeEnabled(event.eventTypeString()))
                return;
    
            if (!options->eventNameEnabled(event.eventName()))
                return;

            break;

        case WatchEvents:
        case WatchNodes:
        case WatchClusters:
        case WatchJobs:
        case WatchContainers:
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
S9sMonitor::eventHandler(
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

    S9sEvent    event = jsonMessage;
    S9sMonitor *display = (S9sMonitor *) userData;

    display->eventCallback(event);
}
