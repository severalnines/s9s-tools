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
#include "s9scommander.h"


#include <unistd.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sCommander::S9sCommander(
        S9sRpcClient            &client) : 
    S9sDisplay(true),
    m_client(client),
    m_rootNodeRecevied(0)
{
    m_leftPanel.setVisible(true);
    m_leftPanel.setSelectionIndex(0);
    m_leftPanel.setHasFocus(true);
    m_leftPanel.setSelectionEnabled(true);

    m_rightPanel.setVisible(true);
    m_rightPanel.setSelectionIndex(0);
    m_rightPanel.setHasFocus(false);
    m_rightPanel.setSelectionEnabled(true);
}

S9sCommander::~S9sCommander()
{
}

void
S9sCommander::main()
{
    start();
    updateTree();

        while (true)
        {
#if 0
            while (!m_client.isAuthenticated())
            {
                m_client.maybeAuthenticate();

                if (!m_client.isAuthenticated())
                    usleep(3000000);
            }

            m_lastReply = S9sRpcReply();
            m_client.subscribeEvents(S9sMonitor::eventHandler, (void *) this);
            m_lastReply = m_client.reply();
#endif
            if (time(NULL) - m_rootNodeRecevied > 10)
                updateTree();

            usleep(500000);
        }    
}

void
S9sCommander::updateTree()
{
    S9sRpcReply     getTreeReply;

    m_client.getTree(true);
    getTreeReply = m_client.reply();
    m_rootNode = getTreeReply.tree();

    m_leftPanel.setCdt(m_rootNode);
    m_rightPanel.setCdt(m_rootNode);

    m_rootNodeRecevied = time(NULL);
}

/**
 * \returns True if the program should continue refreshing the screen, false to
 *   exit.
 */
bool 
S9sCommander::refreshScreen()
{
    startScreen();
    printHeader();

    m_leftPanel.setSize(width() / 2, height() - 2);
    m_rightPanel.setSize(width() / 2, height() - 2);

    for (int idx = 0u; idx < height() - 2; ++idx)
    {
        m_leftPanel.printLine(idx);
        m_rightPanel.printLine(idx);
        printNewLine();
    }

    printFooter();

    return true;
}

void
S9sCommander::processKey(
        int key)
{
    switch (key)
    {
        case 'q':
            exit(0);

        case '\t':
            if (m_leftPanel.hasFocus())
            {
                m_leftPanel.setHasFocus(false);
                m_rightPanel.setHasFocus(true);
            } else {
                m_leftPanel.setHasFocus(true);
                m_rightPanel.setHasFocus(false);
            }
            return;
    }

    if (m_leftPanel.hasFocus())
        m_leftPanel.processKey(key);
    
    if (m_rightPanel.hasFocus())
        m_rightPanel.processKey(key);
}
