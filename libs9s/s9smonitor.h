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

/**
 * Implements a view that can be used to monitor objects through events.
 */
class S9sMonitor : public S9sDisplay
{
    public:
        enum DisplayMode 
        {
            PrintEvents,
            WatchNodes,
            WatchClusters,
            WatchJobs,
            WatchContainers,
            WatchEvents,
        };

        S9sMonitor(S9sMonitor::DisplayMode mode = S9sMonitor::PrintEvents);
        virtual ~S9sMonitor();

        static void eventHandler(
                const S9sVariantMap &jsonMessage,
                void                *userData);

        int nContainers() const;
    protected:

        void eventCallback(S9sEvent &event);

        virtual void processKey(int key);
        virtual void refreshScreen();
        virtual void printHeader();
        virtual void printFooter();
        
        virtual void processEvent(S9sEvent &event);
        void processEventList(S9sEvent &event);
        void removeOldObjects();
        
    private:
        void printContainers();
        void printNodes();
        void printEvents();
        void printClusters();
        void printJobs();

    private:
        DisplayMode                  m_displayMode;
        S9sMap<int, S9sNode>         m_nodes;
        S9sMap<S9sString, S9sServer> m_servers;
        S9sMap<int, S9sCluster>      m_clusters;
        S9sMap<int, S9sJob>          m_jobs;
        S9sMap<int, time_t>          m_jobActivity;
        S9sVector<S9sEvent>          m_events;

        bool                         m_viewDebug;
        int                          m_selectionIndex;
        bool                         m_selectionEnabled;
};

