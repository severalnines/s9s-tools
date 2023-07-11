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
#pragma once

#include <termios.h>

#include "s9sdisplay.h"
#include "s9sformatter.h"
#include "s9srpcreply.h"
#include "s9sprocess.h"
#include "s9ssqlprocess.h"
#include "s9svector.h"

class S9sRpcClient;

/*
 * http://stackoverflow.com/questions/905060/non-blocking-getch-ncurses
 */
class S9sTopUi :
    public S9sDisplay,
    public S9sFormatter
{
    public:
        enum SortOrder
        {
            PidOrder,
            CpuUsage,
            MemUsage,
        };

        enum ViewMode
        {
            // Showing the top OS processes...
            OsProcesses,
            // Showing the top SQL processes...
            SqlProcesses,
        };

        S9sTopUi(
                S9sRpcClient        &client, 
                S9sTopUi::ViewMode   viewMode = OsProcesses);

        virtual ~S9sTopUi();

        virtual void processKey(int key);
        virtual bool processButton(uint button, uint x, uint y);        

        void executeTop();

    protected:
        virtual bool refreshScreen();
        virtual void printHeader();
        virtual void printFooter();
        
        bool getProcesses();
        bool getSqlProcesses();
        void printProcesses(int maxLines);
        void printSqlProcesses(int maxLines);

    private:
        ViewMode               m_viewMode;
        S9sRpcClient          &m_client;
        int                    m_nReplies;

        S9sMutex               m_networkMutex;        
        S9sRpcReply            m_clustersReply;
        time_t                 m_clustersReplyReceived;
        S9sRpcReply            m_cpuStatsReply;
        S9sRpcReply            m_memoryStatsReply;
        S9sRpcReply            m_processReply;
        S9sVector<S9sProcess>     m_processes;
        S9sVector<S9sSqlProcess>  m_sqlProcesses;
        int                       m_clusterId;
        S9sString                 m_clusterName;

        SortOrder              m_sortOrder;
        bool                   m_communicating;
        bool                   m_viewDebug;
        bool                   m_reloadRequested;
};


