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
    m_lastKey1(0)
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
        usleep(100000);
    
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
            if (m_lastKey1 == 'q' || m_lastKey1 == 'Q')
                exit(0);
            
            if (m_lastKey1 == 0x1b || m_lastKey1 == 3)
                exit(0);
        }

        m_mutex.lock();
        refreshScreen();
        m_mutex.unlock();
    } while (!shouldStop());

    return 0;
}

void
S9sDisplay::refreshScreen()
{
    if (m_displayMode == WatchNodes)
    {
        printNodes();
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
            //if (event.eventTypeString() != "EventHost")
            //    return;
            break;
    }

    // optional filtration by clusterId
    if (options->clusterId() > S9S_INVALID_CLUSTER_ID
        && options->clusterId() != event.clusterId())
        return;

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
        m_clusters[cluster.clusterId()] = cluster;
    }

    switch (m_displayMode)
    {
        case PrintEvents:
            processEventList(event);
            break;

        case WatchNodes:
            processEventWatchNodes(event);
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

void
S9sDisplay::processEventWatchNodes(
        S9sEvent &event)
{
    S9sNode  node;
    bool     found = false;

    if (!event.hasHost())
    {
        //S9S_DEBUG("%s", STR(event.toVariantMap().toString()));
        printNodes();
        return;
    }

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

    printNodes();
    //S9S_DEBUG("size: %u", m_nodes.size());
    //S9S_DEBUG("%s", STR(event.toVariantMap().toString()));
}

char
S9sDisplay::rotatingCharacter() const
{
    char charset[] = { '/', '-', '\\',  '|' };

    return charset[m_refreshCounter % 3];
}

void
S9sDisplay::printHeader()
{
    S9sDateTime dt = S9sDateTime::currentDateTime();

    ::printf("%c ", rotatingCharacter());
    ::printf("s9s ");
    ::printf("%s ", STR(dt.toString(S9sDateTime::LongTimeFormat)));
    ::printf("%3lu node(s) ", m_nodes.size());
    ::printf("%3lu cluster(s) ", m_clusters.size());
    ::printf("%08x ", m_lastKey1);

    ::printf("%s", TERM_ERASE_EOL);
    ::printf("\n\r");
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

    if (m_refreshCounter == 0)
    {
        ::printf("%s", TERM_CLEAR_SCREEN);
        ::printf("%s", TERM_HOME);
    } else {
        ::printf("%s", TERM_HOME);
    }

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
    if (/*!options->isNoHeaderRequested()*/true)
    {
        versionFormat.widen("VERSION");
        clusterIdFormat.widen("CID");
        clusterNameFormat.widen("CLUSTER");
        hostNameFormat.widen("HOST");
        portFormat.widen("PORT");

        printf("%s", m_formatter.headerColorBegin());
        printf("STAT ");
        versionFormat.printf("VERSION");
        clusterIdFormat.printf("CID");
        clusterNameFormat.printf("CLUSTER");
        hostNameFormat.printf("HOST");
        portFormat.printf("PORT");
        printf("COMMENT");
        printf("%s\n\r", m_formatter.headerColorEnd());
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
    }

    ::printf("%s", TERM_ERASE_EOL);
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
