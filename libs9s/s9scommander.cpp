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
#include "S9sDateTime"
#include "S9sMutexLocker"

#include <unistd.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sCommander::S9sCommander(
        S9sRpcClient            &client) : 
    S9sDisplay(true),
    m_client(client),
    m_rootNodeRecevied(0),
    m_communicating(false),
    m_reloadRequested(false)
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
    int          updateFreq = 60;
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
            if (time(NULL) - m_rootNodeRecevied > updateFreq || 
                    m_reloadRequested)
            {
                updateTree();
            }

            usleep(100000);
        }    
}

/**
 * Reloads the tree from the controller and pushes it into the widgets.
 */
void
S9sCommander::updateTree()
{
    S9sMutexLocker   locker(m_networkMutex);    
    S9sRpcReply      getTreeReply;

    m_communicating   = true;
    m_reloadRequested = false;

    m_client.getTree(true);
    getTreeReply = m_client.reply();

    m_mutex.lock();     
    m_rootNode = getTreeReply.tree();
    m_leftPanel.setCdt(m_rootNode);
    m_rightPanel.setCdt(m_rootNode);
    m_rootNodeRecevied = time(NULL);
    m_communicating = false;    
    m_mutex.unlock(); 
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

/**
 * \param key The code of the pressed key.
 *
 * This function is called when the user pressed a key on the keyboard.
 */
void
S9sCommander::processKey(
        int key)
{
    switch (key)
    {
        case 'q':
            exit(0);

        case '\t':
            // Tab switches the focus between the left and right panel.
            if (m_leftPanel.hasFocus())
            {
                m_leftPanel.setHasFocus(false);
                m_rightPanel.setHasFocus(true);
            } else {
                m_leftPanel.setHasFocus(true);
                m_rightPanel.setHasFocus(false);
            }
            return;

        case 'd':
        case 'D':
            // Turning on and off the debug mode.
            m_viewDebug = !m_viewDebug;
            break;
    }

    if (m_leftPanel.hasFocus())
        m_leftPanel.processKey(key);
    
    if (m_rightPanel.hasFocus())
        m_rightPanel.processKey(key);
}

/**
 * \param button The mouse button code.
 * \param x The x coordinate measured in characters.
 * \param y The y coordinate measured in characters.
 * \returns True if the mouse event is processed and should not considered by
 *   other widgets.
 */
bool 
S9sCommander::processButton(
        uint button, 
        uint x, 
        uint y)
{
    if (y == 1)
    {
        if (x >= 25 && x <= 27)
        {
            // The reload/abort button.
            if (m_communicating)
            {
                m_communicating = false;
            } else {
                m_reloadRequested = true;
            }

            return true;
        }
    }
    return S9sDisplay::processButton(button, x, y);
}

void
S9sCommander::printHeader()
{
    S9sDateTime dt = S9sDateTime::currentDateTime();
    S9sString   title = "S9S";

    ::printf("%s%-12s%s ", 
            TERM_SCREEN_TITLE_BOLD, 
            STR(title), 
            TERM_SCREEN_TITLE);

    ::printf("%c ", rotatingCharacter());
    ::printf("%s ", STR(dt.toString(S9sDateTime::LongTimeFormat)));

    // Printing the network activity character.
    if (m_communicating || m_reloadRequested)
        ::printf("❌ ");
    else
        ::printf("⟳ ");

    if (m_viewDebug)
    {
        ::printf("0x%02x ",      lastKeyCode());
        ::printf("%02dx%02d ",   width(), height());
        ::printf("%02d:%03d,%03d ", m_lastButton, m_lastX, m_lastY);
    }

    printNewLine();
}
