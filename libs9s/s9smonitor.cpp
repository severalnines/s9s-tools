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
#include "S9sRpcReply"

#include <unistd.h>


//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sMonitor::S9sMonitor(
        S9sRpcClient            &client,
        S9sMonitor::DisplayMode  mode) : 
    S9sDisplay(mode != PrintEvents),
    m_client(client),
    m_displayMode(mode),
    m_viewDebug(false),
    m_viewObjects(false),
    m_fastMode(false),
    m_viewHelp(false),
    m_selectionIndex(0),
    m_selectionEnabled(true),
    m_leftKeyPresses(0),
    m_rightKeyPresses(0)
{
    m_eventListWidget.setSelectionEnabled(false);
    m_eventViewWidget.setActive(false);
    setDisplayMode(mode);
}

S9sMonitor::~S9sMonitor()
{
}

void
S9sMonitor::setDisplayMode(
        const S9sMonitor::DisplayMode mode)
{
    m_displayMode = mode;

    switch (m_displayMode)
    {
        case PrintEvents:
            m_containerListWidget.setVisible(false);
            m_serverListWidget.setVisible(false);
            m_eventListWidget.setVisible(false);

            m_eventViewWidget.setVisible(false);
            break;

        case WatchEvents:
            m_containerListWidget.setVisible(false);
            m_serverListWidget.setVisible(false);
            m_eventListWidget.setVisible(true);
            
            m_eventViewWidget.setVisible(true);
            break;

        case WatchNodes:
        case WatchClusters:
            m_containerListWidget.setVisible(false);
            m_serverListWidget.setVisible(false);
            m_eventListWidget.setVisible(false);
            
            m_eventViewWidget.setVisible(false);
            break;

        case WatchJobs:
            m_containerListWidget.setVisible(false);
            m_serverListWidget.setVisible(false);
            m_eventListWidget.setVisible(false);
            
            m_eventViewWidget.setVisible(false);
            break;

        case WatchServers:
            m_containerListWidget.setVisible(false);
            m_serverListWidget.setVisible(true);
            m_eventListWidget.setVisible(false);
            
            m_eventViewWidget.setVisible(false);
            break;

        case WatchContainers:
            m_containerListWidget.setVisible(true);
            m_serverListWidget.setVisible(false);
            m_eventListWidget.setVisible(false);
            
            m_eventViewWidget.setVisible(false);
            break;
    }
    
}

/**
 * Starts the screen preiodic refresh that will keep updating the terminal from
 * a secondary thread. This method will authenticate if necessary,
 * re-authenticate if the session expires. The code will also subscribe to the
 * events with the callback that will update the data in the monitor object
 * whenever an event is received.
 *
 * This method in its current state will never return, when the user requests
 * exit the exit() function will be called.
 */
void
S9sMonitor::main()
{
    int    nEvents = 0;
    double millis;
    double speedFactor = 1.0;

    start();

    if (hasInputFile())
    {
        S9sDateTime  prevCreated;
        S9sDateTime  thisCreated;
        S9sEvent     event;
        bool         success;

        S9S_DEBUG("Has input file...");
        for (;;)
        {
            while (m_isStopped && m_rightKeyPresses == 0)
                usleep(100000);

            success = m_inputFile.readEvent(event);
            if (!success)
                break;

            if (nEvents == 0)
            {
                thisCreated = event.created();
            } else {
                prevCreated = thisCreated;
                thisCreated = event.created();

                millis  = S9sDateTime::milliseconds(thisCreated, prevCreated);
                millis *= speedFactor;

                if (millis > 500)
                    millis = 500;

                if (m_rightKeyPresses == 0)
                    usleep(millis * 1000);
            }
           
            if (m_rightKeyPresses > 0)
            {
                S9sMutexLocker locker(m_mutex);
                int    skipSeconds = m_rightKeyPresses * 60 * 3;
                time_t target = thisCreated.toTimeT() + skipSeconds;

                do {
                    processEvent(event);

                    success = m_inputFile.readEvent(event);
                    if (!success)
                        break;
                
                    prevCreated = thisCreated;
                    thisCreated = event.created();
                    ++nEvents;
                } while (thisCreated.toTimeT() < target);
                
                m_rightKeyPresses = 0;
                refreshScreen();
            }

            while (m_isStopped && m_rightKeyPresses == 0)
                usleep(100000);

            m_mutex.lock();
            processEvent(event);
            m_mutex.unlock();
            ++nEvents;
        }
    } else {
        while (true)
        {
            while (!m_client.isAuthenticated())
            {
                m_client.maybeAuthenticate();

                if (!m_client.isAuthenticated())
                    usleep(3000000);
            }

            m_lastReply = S9sRpcReply();
            m_client.subscribeEvents(S9sMonitor::eventHandler, (void *) this);
            m_lastReply = m_client.reply();
            sleep(1);
        }
    }
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

S9sVector<S9sServer>
S9sMonitor::servers() const
{
    S9sVector<S9sServer> retval;

    foreach (const S9sServer &server, m_servers)
        retval << server;

    sort(retval.begin(), retval.end(), S9sServer::compareByName);

    return retval;
}

/**
 * \returns True if the program should continue refreshing the screen, false to
 *   exit.
 *
 * Virtual function that should paint the whole screen once. The mutex is 
 * locked when this method is called.
 */
bool
S9sMonitor::refreshScreen()
{
    if (!hasInputFile())
    {
        if (!m_client.isAuthenticated() || 
                (!m_lastReply.empty() && !m_lastReply.isOk()))
        {
            S9sString message;
    
            if (!m_lastReply.isOk() && !m_lastReply.errorString().empty())
            {
                message.sprintf("*** %s ***", STR(m_lastReply.errorString()));
            } else if (!m_client.errorString().empty())
            {
                message.sprintf("*** %s ***", STR(m_client.errorString()));
            } else {
                message.sprintf("*** Not connected. ***");
            }

            startScreen();
            printHeader();
            printMiddle(message);
            printFooter();
            return true;
        }
    }

    switch (m_displayMode)
    {
        case WatchNodes:
            printNodes();
            break;
        
        case WatchContainers:
            printContainers();
            break;
        
        case WatchServers:
            printServers();
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

        default:
            ::printf("error");
    }

    //if (m_viewHelp)
    //    printHelp();

    return true;
}

void
S9sMonitor::printHelp()
{
    S9sVariantList lines;
    int            indent;

    lines <<
        "┌─────────────────────────────────────────────────────────────────┐" <<
        "│                             VIEWS                               │" <<
        "│                                                                 │" <<
        "│     N - Nodes             J - Jobs           S - Servers        │" <<
        "│     C - Clusters          E - Events         V - Containers     │" <<
        "│                                                                 │" <<
        "│                                                                 │" <<
        "│     H - Toggle help screen.                                     │" <<
        "│     D - Debug mode.                                             │" <<
        "│     O - View objects.                                           │" <<
        "│     Q - Quit program.                                           │" <<
        "│                                                                 │" <<
        "│                                                                 │" <<
        "│                                                                 │" <<
        "│                     PLAYBACK CONTROL KEYS                       │" <<
        "│                                                                 │" <<
        "│ SPACE - Start and stop playback.                                │" <<
        "│     F - Toggle fast playback mode.                              │" <<
        "│                                                                 │" <<
        "│                                                                 │" <<
        "│                                                                 │" <<
        "│                                                                 │" <<
        "└─────────────────────────────────────────────────────────────────┘";
        ;

    indent = (columns() - 65) / 2;
    if (indent < 2) 
        indent = 2;

    for (int n = 0; n < (int)lines.size(); ++n)
    {
        S9sString line = lines[n].toString();
        
        gotoXy(indent, n + 3);
        ::printf("%s", STR(line));
    }
}

/**
 * Screen refresh for the "containers" mode.
 */
void
S9sMonitor::printContainers()
{
    S9sVector<S9sServer> theServers = servers();
    S9sFormat  typeFormat;
    S9sFormat  templateFormat;
    S9sFormat  stateFormat;
    S9sFormat  ipFormat;
    S9sFormat  serverFormat;
    S9sFormat  aliasFormat;
    int        totalIndex;


    startScreen();
    printHeader();
    
    for (uint idx1 = 0u; idx1 < theServers.size(); ++idx1)
    {
        S9sServer      &server = theServers[idx1];
        S9sVariantList  maps = server.containers();

        for (uint idx = 0u; idx < maps.size(); ++idx)
        {
            S9sContainer container = maps[idx].toVariantMap();
            S9sString    ipAddress = container.ipAddress(
                    S9s::AnyIpv4Address, "-");

            typeFormat.widen(container.provider());
            templateFormat.widen(container.templateName("-", true));
            stateFormat.widen(container.state());
            ipFormat.widen(ipAddress);
            serverFormat.widen(container.parentServerName());
            aliasFormat.widen(container.alias());
        }
    }
    
    if (nContainers() > 0)
    {
        typeFormat.widen("CLOUD");
        templateFormat.widen("TEMPLATE");
        stateFormat.widen("STATE");
        ipFormat.widen("IP ADDRESS");
        serverFormat.widen("SERVER");
        aliasFormat.widen("NAME");
        
        printf("%s", TERM_SCREEN_HEADER);
        typeFormat.printf("CLOUD");
        templateFormat.printf("TEMPLATE");
        stateFormat.printf("STATE");
        ipFormat.printf("IP ADDRESS");
        serverFormat.printf("SERVER");
        aliasFormat.printf("NAME");

        printNewLine();
    } else {
        printMiddle("*** No containers. ***");
    }

    /*
     * Printing the list.
     */
    m_containerListWidget.setLocation(1, 3);
    m_containerListWidget.setSize(columns(), rows() - 3);
    m_containerListWidget.setNumberOfItems(nContainers());
    m_containerListWidget.ensureSelectionVisible();

    totalIndex = 0;
    for (uint idx1 = 0u; idx1 < theServers.size(); ++idx1)
    {
        S9sServer      &server = theServers[idx1];
        S9sVariantList maps = server.containers();

        for (uint idx = 0u; idx < maps.size(); ++idx)
        {
            S9sContainer container = maps[idx].toVariantMap();
            S9sString    ipAddress = container.ipAddress(
                    S9s::AnyIpv4Address, "-");
            int          stateAsChar = container.stateAsChar();
            bool         selected;
           
            if (!m_containerListWidget.isVisible(totalIndex))
            {
                ++totalIndex;
                continue;
            }

            selected = 
                m_containerListWidget.isSelected(totalIndex) &&
                m_selectionEnabled;

            if (!selected)
            {
                typeFormat.printf(STR(container.provider()));
                templateFormat.printf(container.templateName("-", true));
                stateFormat.printf(STR(container.state()));

                ::printf("%s", ipColorBegin(ipAddress));
                ipFormat.printf(STR(ipAddress));
                ::printf("%s", ipColorEnd(ipAddress));

                ::printf("%s", serverColorBegin());
                serverFormat.printf(container.parentServerName());
                ::printf("%s", serverColorEnd());

                ::printf("%s", containerColorBegin(stateAsChar));
                aliasFormat.printf(container.alias());
                ::printf("%s", containerColorEnd());
            } else {
                // The line is selected, we use a highlight color.
                ::printf("%s", XTERM_COLOR_SELECTION);
                typeFormat.printf(STR(container.provider()));
                templateFormat.printf(container.templateName("-"));
                stateFormat.printf(STR(container.state()));
                ipFormat.printf(STR(ipAddress));
                serverFormat.printf(container.parentServerName());
                aliasFormat.printf(container.alias());
            }

            ++totalIndex;
            printNewLine();
            
            if (m_lineCounter >= rows() - 1)
                break;
        }   
            
        if (m_lineCounter >= rows() - 1)
            break;
    }
    
    printFooter();
}

/**
 * Screen refresh function for the "servers" mode.
 */
void
S9sMonitor::printServers()
{
    S9sVector<S9sServer> theServers = servers();
    S9sFormat   sourceFileFormat(XTERM_COLOR_BLUE, TERM_NORMAL);
    S9sFormat   sourceLineFormat;
    S9sFormat   idFormat;
    S9sFormat   typeFormat;
    S9sFormat   versionFormat;
    S9sFormat   nContainersFormat;
    S9sFormat   ownerFormat(userColorBegin(), userColorEnd());
    S9sFormat   groupFormat(groupColorBegin(), groupColorEnd());
    S9sFormat   nameFormat;
    S9sFormat   ipFormat(ipColorBegin(), ipColorEnd());
    S9sFormat   commentsFormat;

    startScreen();
    printHeader();
    
    for (uint idx1 = 0u; idx1 < theServers.size(); ++idx1)
    {
        S9sServer      &server = theServers[idx1];
        S9sEvent        event = m_serverEvents[server.id()];
        
        sourceFileFormat.widen(event.senderFile());
        sourceLineFormat.widen(event.senderLine());
        idFormat.widen(server.id("-"));
        typeFormat.widen(server.protocol());
        versionFormat.widen(server.version("-"));
        nContainersFormat.widen(server.nContainers());
        ownerFormat.widen(server.ownerName());
        groupFormat.widen(server.groupOwnerName());
        nameFormat.widen(server.hostName());
        ipFormat.widen(server.ipAddress());
        commentsFormat.widen(server.message("-"));
    }
    
    /*
     * Printing the header.
     */
    if (!m_servers.empty())
    {
        sourceFileFormat.widen("SOURCE FILE");
        sourceLineFormat.widen("LINE");
        
        idFormat.widen("ID");

        typeFormat.widen("CLD");
        versionFormat.widen("VERSION");
        nContainersFormat.widen("#C");
        ownerFormat.widen("OWNER");
        groupFormat.widen("GROUP");
        nameFormat.widen("HOSTNAME");
        ipFormat.widen("IPADDRESS");
        commentsFormat.widen("COMMENT");

        printf("%s", TERM_SCREEN_HEADER);
        
        if (m_viewDebug)
        {
            sourceFileFormat.printf("SOURCE FILE", false);
            sourceLineFormat.printf("LINE");
        }

        if (m_viewObjects)
        {
            idFormat.printf("ID");
        }

        typeFormat.printf("CLD", false);
        versionFormat.printf("VERSION", false);
        nContainersFormat.printf("#C", false);
        ownerFormat.printf("OWNER", false);
        groupFormat.printf("GROUP", false);
        nameFormat.printf("HOSTNAME", false);
        ipFormat.printf("IPADDRESS", false);
        commentsFormat.printf("COMMENT", false);

        printNewLine();
    } else {
        printMiddle("*** No servers. ***");
    }

    /*
     * Printing the list.
     */
    m_serverListWidget.setLocation(1, 3);
    m_serverListWidget.setSize(columns(), rows() - 3);
    m_serverListWidget.setNumberOfItems(theServers.size());
    m_serverListWidget.ensureSelectionVisible();
    for (uint idx1 = 0u; idx1 < theServers.size(); ++idx1)
    {
        S9sServer      &server = theServers[idx1];
        S9sEvent        event = m_serverEvents[server.id()];
        bool            isSelected;

        if (!m_serverListWidget.isVisible(idx1))
            continue;

        isSelected = 
                m_serverListWidget.isSelected(idx1) &&
                m_selectionEnabled;

        nameFormat.setColor(
                server.colorBegin(true),
                server.colorEnd(true));

        if (isSelected)
        {
            ::printf("%s", XTERM_COLOR_SELECTION);

            if (m_viewDebug)
            {
                sourceFileFormat.printf(event.senderFile(), false);
                sourceLineFormat.printf(event.senderLine());
            }

            if (m_viewObjects)
                idFormat.printf(server.id("-"), false);

            typeFormat.printf(server.protocol(), false);
            versionFormat.printf(server.version("-"), false);
            nContainersFormat.printf(server.nContainers());
            ownerFormat.printf(server.ownerName(), false);
            groupFormat.printf(server.groupOwnerName(), false);
            nameFormat.printf(server.hostName(), false);
            ipFormat.printf(server.ipAddress(), false);
            commentsFormat.printf(server.message("-"), false);
        } else {
            if (m_viewDebug)
            {
                sourceFileFormat.printf(event.senderFile());
                sourceLineFormat.printf(event.senderLine());
            }

            if (m_viewObjects)
                idFormat.printf(server.id("-"));

            typeFormat.printf(server.protocol());
            versionFormat.printf(server.version("-"));
            nContainersFormat.printf(server.nContainers());
            ownerFormat.printf(server.ownerName());
            groupFormat.printf(server.groupOwnerName());
            nameFormat.printf(server.hostName());
            ipFormat.printf(server.ipAddress());
            commentsFormat.printf(server.message("-"));
        }

        printNewLine();
            
        if (m_lineCounter >= rows() - 1)
            break;
    }
    
    printFooter();
}


/**
 * Screen refresh method for the "clusters" mode.
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
    S9sFormat aclFormat;
    S9sFormat ownerFormat(userColorBegin(), userColorEnd());
    S9sFormat groupFormat(groupColorBegin(), groupColorEnd());
    S9sFormat pathFormat(ipColorBegin(), ipColorEnd());

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
        
        aclFormat.widen("c" + cluster.aclShortString());
        ownerFormat.widen(cluster.ownerName());
        groupFormat.widen(cluster.groupOwnerName());
        pathFormat.widen(cluster.fullCdtPath());
    }

    if (!m_clusters.empty())
    {
        versionFormat.widen("VERSION");
        idFormat.widen("ID");
        stateFormat.widen("STATE");
        typeFormat.widen("TYPE");
        nameFormat.widen("NAME");
        messageFormat.widen("MESSAGE");
        
        aclFormat.widen("MODE");
        ownerFormat.widen("OWNER");
        groupFormat.widen("GROUP");
        pathFormat.widen("PATH");

        printf("%s", TERM_SCREEN_HEADER);
        
        if (m_viewObjects)
        {
            aclFormat.printf("MODE", false);
            ownerFormat.printf("OWNER", false);
            groupFormat.printf("GROUP", false);
            pathFormat.printf("PATH", false);
        } else {
            versionFormat.printf("VERSION");
            idFormat.printf("ID");
            stateFormat.printf("STATE");
            typeFormat.printf("TYPE");
            nameFormat.printf("NAME");
            messageFormat.printf("MESSAGE");
        }
        
        printNewLine();
    } else {
        printMiddle("*** No clusters. ***");
    }

    /*
     * Printing.
     */
    foreach (const S9sCluster &cluster, m_clusters)
    {
        if (m_viewObjects)
        {
            aclFormat.printf("n" + cluster.aclShortString());
            ownerFormat.printf(cluster.ownerName());
            groupFormat.printf(cluster.groupOwnerName());
            pathFormat.printf(cluster.fullCdtPath());
        } else {
            versionFormat.printf(cluster.vendorAndVersion());
            idFormat.printf(cluster.clusterId());
        
            printf("%s", clusterStateColorBegin(cluster.state()));
            stateFormat.printf(cluster.state());
            printf("%s", clusterStateColorEnd());

            typeFormat.printf(cluster.clusterType());
    
            printf("%s", clusterColorBegin());
            nameFormat.printf(cluster.name());
            printf("%s", clusterColorEnd());
        
            messageFormat.printf(cluster.statusText());
        }

        printNewLine();
        
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
        printNewLine();
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

        stateFormat.setColor(
                jobStateColorBegin(job.status()),
                jobStateColorEnd());

        titleFormat.setColor(
                TERM_BOLD, TERM_NORMAL);

        idFormat.printf(job.id());
        stateFormat.printf(job.status());

        ::printf("%s", STR(progressBar));

        titleFormat.printf(job.title());
        statusTextFormat.printf(statusText);

        printNewLine();
        
        if (m_lineCounter >= rows() - 1)
            break;
    }

    printFooter();
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
    S9sFormat   ownerFormat(userColorBegin(), userColorEnd());
    S9sFormat   groupFormat(groupColorBegin(), groupColorEnd());
    S9sFormat   pathFormat(ipColorBegin(), ipColorEnd());
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
        
        aclFormat.widen("MODE");
        ownerFormat.widen("OWNER");
        groupFormat.widen("GROUP");
        pathFormat.widen("PATH");

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

        printNewLine();
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

            printf("%s", clusterColorBegin());
            clusterNameFormat.printf(clusterName);
            printf("%s", clusterColorEnd());

            hostNameFormat.printf(node.hostName());
            portFormat.printf(node.port());

            ::printf("%s ", STR(node.message()));
        }

        printNewLine();
    }

    printFooter();
}

/**
 * Printing method for the event view.
 */
void
S9sMonitor::printEvents()
{
    startScreen();
    printHeader();

    if (m_events.empty())
    {
        printMiddle("*** No events. ***");
    } else {
        printEventList();
        printEventView();
    }

    printFooter();
}

void
S9sMonitor::printEventList()
{
    int firstIndex, lastIndex;
    int viewHeight;

    if (m_eventListWidget.selectionIndex() < (int) m_events.size() &&
            m_eventListWidget.selectionIndex() >= 0)
        m_selectedEvent = m_events[m_eventListWidget.selectionIndex()];

    /*
     * Layout.
     */
    viewHeight = 
        m_eventViewWidget.isActive() ? (rows() - 2) / 2 : rows() - 2;

    m_eventListWidget.setLocation(1, 2);
    m_eventListWidget.setSize(columns(), viewHeight);
    m_eventListWidget.setNumberOfItems(m_events.size());
    m_eventListWidget.ensureSelectionVisible();

    m_eventViewWidget.setLocation(1, viewHeight + 1);
    m_eventViewWidget.setSize(columns(), viewHeight);
    m_eventViewWidget.setSelectionEnabled(false);

    /*
     * Printing the list.
     */
    firstIndex = m_eventListWidget.firstVisibleIndex();
    lastIndex  = m_eventListWidget.lastVisibleIndex();
    for (uint idx = firstIndex; (int) idx <= lastIndex; ++idx)
    {

        if (idx >= m_events.size())
            break;

        S9sEvent  &event = m_events[idx];
        S9sString  line;
        bool       isSelected;
        
        isSelected = m_eventListWidget.isSelected(idx);

        line = event.toOneLiner(!isSelected, m_viewDebug);

        line.replace("\n", "\\n");
        line.replace("\r", "\\r");
       
        if (isSelected)
        {
            ::printf("%s", XTERM_COLOR_SELECTION);
            ::printf("%s ", STR(line));
            printNewLine();
        } else {
            ::printf("%s ", STR(line));
            printNewLine();
        }
    }

    // The remaining lines of the event list.
    while (m_lineCounter < 
            m_eventListWidget.y() + m_eventListWidget.height() - 1)
    {
        #if 0
        ::printf("%3d %3d %3d", m_lineCounter, m_eventListWidget.y(), 
                m_eventListWidget.height());
        #endif
        printNewLine();
    }
}

void
S9sMonitor::printEventView()
{
    if (!m_eventViewWidget.isActive())
        return;

    S9sString title = " Event JSon";

    // The title bar.
    ::printf("%s", TERM_INVERSE);
    ::printf("%s", STR(title));

#if 1
    for (int n = title.length(); n < columns() - 2; ++n)
        ::printf(" ");

    ::printf("x ");
#else
    ::printf("  %d, %d %dx%d %d - %d", 
            m_eventViewWidget.x(), m_eventViewWidget.y(),
            m_eventViewWidget.height(), m_eventViewWidget.width(),
            m_eventViewWidget.firstVisibleIndex(),
            m_eventViewWidget.lastVisibleIndex()
            );
#endif
    printNewLine();
   
    S9sVariantList lines = m_selectedEvent.toString().split("\n");
    uint startLineIdx, endLineIdx;

    m_eventViewWidget.setNumberOfItems(lines.size());
    m_eventViewWidget.ensureSelectionVisible();

    startLineIdx = m_eventViewWidget.firstVisibleIndex();
    endLineIdx   = m_eventViewWidget.lastVisibleIndex();
    for (uint idx = startLineIdx; idx < endLineIdx; ++idx)
    {
        if (idx >= lines.size())
            break;

        S9sString line = lines[idx].toString();

        line.replace("\n", "\\n");
        line.replace("\r", "\\r");
        ::printf("%s", STR(line));
        printNewLine();

    }
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

    while (m_events.size() > 3000)
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
        S9sServer  server = event.server();

        if (event.eventSubClass() == S9sEvent::Destroyed)
        {
            m_servers.erase(server.id());
            m_serverEvents.erase(server.id());
        } else {
            m_servers[server.id()]      = server;
            m_serverEvents[server.id()] = event;
        }
    }

    removeOldObjects();
    if (m_rightKeyPresses > 0)
        return;

    switch (m_displayMode)
    {
        case PrintEvents:
            processEventList(event);
            break;

        case WatchNodes:
            if (!m_viewHelp)
                printNodes();
            ++m_refreshCounter;
            break;
        
        case WatchContainers:
            if (!m_viewHelp)
                printContainers();
            ++m_refreshCounter;
            break;
        
        case WatchServers:
            if (!m_viewHelp)
                printServers();
            ++m_refreshCounter;
            break;

        case WatchClusters:
            if (!m_viewHelp)
                printClusters();
            ++m_refreshCounter;
            break;
        
        case WatchJobs:
            if (!m_viewHelp)
                printJobs();
            ++m_refreshCounter;
            break;
        
        case WatchEvents:
            if (!m_viewHelp)
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
        output = event.toOneLiner(true, options->isDebug());
    }

    output.replace("\n", "\n\r");
    if (!output.empty())
        ::printf("\n\r%s", STR(output));
}

/**
 * This method is called from time to time to remove the old objects e.g. the
 * jobs that are finished a while ago.
 */
void
S9sMonitor::removeOldObjects()
{
    // FIXME: this is disabled
    return;
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
    const char *bold = TERM_SCREEN_TITLE_BOLD;
    const char *normal = TERM_SCREEN_TITLE;

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
        
        case WatchServers:
            title = "S9S SERVER VIEW    ";
            break;
        
        case WatchEvents:
            title = "S9S EVENT VIEW     ";
            break;
            
        default:
            break;
    }

    ::printf("%s%s%s ", bold, STR(title), normal);
    ::printf("%c ", rotatingCharacter());
    
    if (hasInputFile())
    {
        if (m_isStopped)
        {
            if (m_fastMode)
                ::printf(" ⏩ ");
            else
                ::printf(" ▶️ ");
        } else {
            ::printf(" ⏸️ ");
        }
    } else {
        ::printf("   ");
    }

    //::printf("⏺ ⏹ ⏸ ⏵ ⏩");

    ::printf("%s ", STR(dt.toString(S9sDateTime::LongTimeFormat)));
    
    ::printf("%s%4zu%s event(s) ", bold, m_events.size(), normal);
    ::printf("%s%zu%s node(s) ",   bold, m_nodes.size(), normal);
    ::printf("%s%d%s VM(s) ",      bold, nContainers(), normal);
    ::printf("%s%zu%s cluster(s) ", bold, m_clusters.size(), normal);
    ::printf("%s%zu%s jobs(s) ",   bold, m_jobs.size(), normal);

    if (m_viewDebug)
    {
        ::printf("0x%08x ",      lastKeyCode());
        ::printf("%02dx%02d ",   columns(), rows());
        ::printf("%02d:%03d,%03d ", m_lastButton, m_lastX, m_lastY);
    }

    printNewLine();
}

/**
 * Printing the bottom part of the screen.
 */
void
S9sMonitor::printFooter()
{
    const char *bold   = TERM_SCREEN_TITLE_BOLD;
    const char *normal = TERM_SCREEN_TITLE;

    //::printf("%s", TERM_ERASE_EOL);
    for (;m_lineCounter < rows() - 1; ++m_lineCounter)
    {
        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\n\r");
        ::printf("%s", TERM_ERASE_EOL);
    } 

    ::printf("%s ", normal);
    ::printf("%sN%s-Nodes ", bold, normal);
    ::printf("%sC%s-Clusters ", bold, normal);
    ::printf("%sJ%s-Jobs ", bold, normal);
    ::printf("%sV%s-Containers ", bold, normal);
    ::printf("%sE%s-Events ", bold, normal);
    ::printf("%sD%s-Debug mode ", bold, normal);
    ::printf("%sH%s-Help ", bold, normal);
    ::printf("%sQ%s-Quit", bold, normal);
   
    //if (!m_outputFileName.empty())
    //    ::printf("    [%s]", STR(m_outputFileName));
    //    ::printf("    {%s}", STR(m_inputFileName));

    // Just for debugging now.
    //::printf("'%s'", STR(m_client.reply().requestStatusAsString()));
    // No new-line at the end, this is the last line.
    ::printf("%s", TERM_ERASE_EOL);
    ::printf("%s", TERM_NORMAL);

    if (m_viewHelp)
        printHelp();
    
    fflush(stdout);
}

/**
 * Virtual function that called when the user pressed a key on the keyboard.
 */
void
S9sMonitor::processKey(
        int key)
{
    switch (key)
    {
        case 'q':
        case 'Q':
            exit(0);
            break;

        case 0x0d: 
            // The enter key...
            if (m_displayMode == WatchEvents)
            {
                if (!m_eventListWidget.isSelectionEnabled())
                    m_eventListWidget.setSelectionEnabled(true);
                else
                    m_eventViewWidget.setActive(true);
            }
            break;

        case 'x': 
            if (m_displayMode == WatchEvents)
                m_eventViewWidget.setActive(false);
            break;
            
        case 0x1b:
            // The ESC key.
            if (m_viewHelp)
            {
                m_viewHelp = false;
            } else if (m_displayMode == WatchEvents)
            {
                if (m_eventViewWidget.isActive())
                    m_eventViewWidget.setActive(false);
                else if (m_eventListWidget.isSelectionEnabled())
                    m_eventListWidget.setSelectionEnabled(false);
                else
                    exit(0);
            } else {
                exit(0);
            }
            break;

        case 3:
            exit(0);
            break;

        case 'c':
        case 'C':
            setDisplayMode(WatchClusters);
            break;
       
        case 'd':
        case 'D':
            // Turning on and off the debug mode.
            m_viewDebug = !m_viewDebug;
            break;

        case 'o':
        case 'O':
            m_viewObjects = !m_viewObjects;
            break;

        case 'n':
        case 'N':
            setDisplayMode(WatchNodes);
            break;
        
        case 'j':
        case 'J':
            setDisplayMode(WatchJobs);
            break;
        
        case 'v':
        case 'V':
            setDisplayMode(WatchContainers);
            break;
        
        case 's':
        case 'S':
            setDisplayMode(WatchServers);
            break;
        
        case 'e':
        case 'E':
            setDisplayMode(WatchEvents);
            break;
        
        case 'f':
        case 'F':
            m_fastMode = !m_fastMode;
            break;
        
        case 'h':
        case 'H':
            m_viewHelp = !m_viewHelp;
            break;

        case ' ':
            m_isStopped = !m_isStopped;
            break;

        case S9S_KEY_LEFT:
            m_leftKeyPresses++;
            break;

        case S9S_KEY_RIGHT:
            m_rightKeyPresses++;
            break;

        default:
            m_serverListWidget.processKey(key);
            m_containerListWidget.processKey(key);
            m_eventListWidget.processKey(key);
    }
}

/**
 * \param button The mouse button code.
 * \param x The x coordinate measured in characters.
 * \param y The y coordinate measured in characters.
 *
 * Virtual function that is called when the user pressed a mouse button.
 */
void 
S9sMonitor::processButton(
        uint button, 
        uint x, 
        uint y)
{
    S9sDisplay::processButton(button, x, y);

    m_eventViewWidget.processButton(button, x, y);

    if (m_containerListWidget.processButton(button, x, y))
        return;
    else if (m_serverListWidget.processButton(button, x, y))
        return;
    else if (m_eventListWidget.processButton(button, x, y))
        return;
    //else if (m_eventViewWidget.processButton(button, x, y))
    //    return;

    if ((int) y == rows())
    {
        if (x >= 2 && x <= 8)
        {
            setDisplayMode(WatchNodes);
        } else if (x >= 10 && x <= 19)
        {
            setDisplayMode(WatchClusters);
        } else if (x >= 21 && x <= 26)
        {
            setDisplayMode(WatchJobs);
        } else if (x >= 28 && x <= 39)
        {
            setDisplayMode(WatchContainers);
        } else if (x >= 41 && x <= 48)
        {
            setDisplayMode(WatchEvents);
        } else if (x >= 50 && x <= 61)
        {
            m_viewDebug = !m_viewDebug;
        } else if (x >= 63 && x <= 68)
        {
            m_viewHelp = !m_viewHelp;
        } else if (x >= 70 && x <= 75)
        {
            exit(0);
        }
    }
}

void
S9sMonitor::replyCallback(
        S9sRpcReply   &reply)
{
    m_lastReply = reply;
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
        case WatchServers:
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

/**
 * Static callback function for the event processing.
 */
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
        // Not an event.
        S9sRpcReply   reply;
        S9sMonitor *display = (S9sMonitor *) userData;
       
        reply = jsonMessage;
        display->replyCallback(reply);
    } else {
        S9sEvent    event = jsonMessage;
        S9sMonitor *display = (S9sMonitor *) userData;

        display->eventCallback(event);
    }
}
