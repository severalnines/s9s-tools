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

#include "S9sDisplay"
#include "S9sRpcReply"

class S9sRpcClient;

/*
 * http://stackoverflow.com/questions/905060/non-blocking-getch-ncurses
 */
class S9sTopUi :
    public S9sDisplay
{
    public:
        S9sTopUi(S9sRpcClient &client);
        virtual ~S9sTopUi();

        void executeTop();

    protected:
        virtual void processKey(int key);
        virtual bool refreshScreen();
        virtual void printHeader();
        virtual void printFooter();
        
        bool executeTopOnce();
        void printProcesses();

    private:
        S9sRpcClient &m_client;
        int           m_nReplies;
        S9sRpcReply   m_clustersReply;
        S9sRpcReply   m_cpuStatsReply;
        S9sRpcReply   m_memoryStatsReply;
        S9sRpcReply   m_processReply;
        int           m_clusterId;
        S9sString     m_clusterName;
};


