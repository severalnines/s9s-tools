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
#pragma once

#include "S9sDisplay"
#include "S9sFormatter"
#include "S9sRpcClient"
#include "S9sRpcReply"
#include "S9sDisplayList"

/**
 * Implements a view that can be used to monitor objects through events.
 */
class S9sMonitor : 
    public S9sDisplay, 
    public S9sFormatter
{
    public:
        enum DisplayMode 
        {
            PrintEvents,
            WatchNodes,
            WatchClusters,
            WatchJobs,
            WatchContainers,
            WatchServers,
            WatchEvents,
        };

        S9sMonitor(
                S9sRpcClient            &client,
                S9sMonitor::DisplayMode  mode = S9sMonitor::PrintEvents);
        virtual ~S9sMonitor();

        virtual void processKey(int key);
        virtual bool processButton(uint button, uint x, uint y);

        static void eventHandler(
                const S9sVariantMap &jsonMessage,
                void                *userData);

        int nContainers() const;
        S9sVector<S9sServer> servers() const;

        void setDisplayMode(const S9sMonitor::DisplayMode mode);

        virtual void main();

    protected:
        void replyCallback(S9sRpcReply &reply);
        void eventCallback(S9sEvent &event);

        virtual bool refreshScreen();
        virtual void printHeader();
        virtual void printFooter();
        
        virtual void processEvent(S9sEvent &event);
        void processEventList(S9sEvent &event);
        //void removeOldObjects();
        
    private:
        void printHelp();
        void printContainers();
        void printServers();
        void printNodes();
        
        void printEvents();
        void printEventList();
        void printEventView();

        void printClusters();
        void printJobs();

    private:
        S9sRpcClient                &m_client;
        S9sRpcReply                  m_lastReply;
        DisplayMode                  m_displayMode;
        S9sMap<int, S9sNode>         m_nodes;
        S9sMap<int, S9sEvent>        m_eventsForNodes;
        S9sMap<S9sString, S9sServer> m_servers;
        S9sMap<S9sString, S9sEvent>  m_serverEvents;
        S9sMap<int, S9sCluster>      m_clusters;
        S9sMap<int, S9sJob>          m_jobs;
        S9sMap<int, time_t>          m_jobActivity;
        S9sVector<S9sEvent>          m_events;

        bool                         m_viewDebug;
        bool                         m_viewObjects;
        bool                         m_fastMode;
        bool                         m_viewHelp;

        int                          m_selectionIndex;
        bool                         m_selectionEnabled;
        int                          m_leftKeyPresses;
        int                          m_rightKeyPresses;

        S9sDisplayList               m_nodeListWidget;
        S9sDisplayList               m_nodeViewWidget;

        S9sDisplayList               m_containerListWidget;
        S9sDisplayList               m_serverListWidget;
        

        S9sDisplayList               m_eventListWidget;
        S9sDisplayList               m_eventViewWidget;

        S9sEvent                     m_selectedEvent;
};

