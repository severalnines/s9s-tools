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

#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sDisplay::S9sDisplay(
        S9sDisplay::DisplayMode mode) :
    m_displayMode(mode),
    m_refreshCounter(0)
{
}

S9sDisplay::~S9sDisplay()
{
}

void 
S9sDisplay::eventCallback(
        S9sEvent &event)
{
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

    if (!output.empty())
        ::printf("%s\n", STR(output));
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
    ::printf("%c ", rotatingCharacter());
    ::printf("%3lu node(s) ", m_nodes.size());
    ::printf("\n");
}

void
S9sDisplay::printNodes()
{
    S9sFormat   versionFormat;
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

    for (uint idx = 0u; idx < m_nodes.size(); ++idx)
    {
        const S9sNode &node = m_nodes[idx];
        
        versionFormat.widen(node.version());
        clusterIdFormat.widen(node.clusterId());
        hostNameFormat.widen(node.hostName());
        portFormat.widen(node.port());
    }

    for (uint idx = 0u; idx < m_nodes.size(); ++idx)
    {
        const S9sNode &node = m_nodes[idx];

        beginColor = S9sRpcReply::hostStateColorBegin(node.hostStatus());
        endColor   = S9sRpcReply::hostStateColorEnd();
        hostNameFormat.setColor(beginColor, endColor);

        ::printf("%c", node.nodeTypeFlag());
        ::printf("%c", node.hostStatusFlag());
        ::printf("%c", node.roleFlag());
        ::printf("%c ", node.maintenanceFlag());
        versionFormat.printf(node.version());
        clusterIdFormat.printf(node.clusterId());
        hostNameFormat.printf(node.hostName());
        portFormat.printf(node.port());

        ::printf("%s ", STR(node.message()));
        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\n");
    }

    ::printf("%s", TERM_ERASE_EOL);
    ++m_refreshCounter;
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
