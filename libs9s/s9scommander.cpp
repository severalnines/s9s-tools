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
    m_leftBrowser.setVisible(true);
    m_leftBrowser.setSelectionIndex(0);
    m_leftBrowser.setHasFocus(true);
    m_leftBrowser.setSelectionEnabled(true);

    m_rightBrowser.setVisible(false);
    m_rightBrowser.setSelectionIndex(0);
    m_rightBrowser.setHasFocus(false);
    m_rightBrowser.setSelectionEnabled(true);
    
    m_rightInfo.setVisible(true);
    m_rightInfo.setHasFocus(false);
    m_rightInfo.setInfoController(
            client.hostName(), client.port(), client.useTls());
    
    
    m_leftInfo.setVisible(false);
    m_leftInfo.setHasFocus(false);
    m_leftInfo.setInfoController(
            client.hostName(), client.port(), client.useTls());
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
            #if 1
            while (!m_client.isAuthenticated())
            {
                m_client.maybeAuthenticate();

                if (!m_client.isAuthenticated())
                    usleep(3000000);
            }

            //m_lastReply = S9sRpcReply();
            //m_client.subscribeEvents(S9sMonitor::eventHandler, (void *) this);
            //m_lastReply = m_client.reply();
            #endif
            if (time(NULL) - m_rootNodeRecevied > updateFreq || 
                    m_reloadRequested)
            {
                updateTree();
            }

            updateObject();
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

    // Updating the screen.
    m_mutex.lock();
    m_rightInfo.setInfoRequestName("getTree");
    m_leftInfo.setInfoRequestName("getTree");
    m_mutex.unlock();

    m_communicating   = true;
    m_reloadRequested = false;

    m_client.getTree(true);
    getTreeReply = m_client.reply();
    
    // Updatng the screen.
    m_mutex.lock();
    m_rightInfo.setInfoRequestName("");
    m_leftInfo.setInfoRequestName("");
    m_rightInfo.setInfoLastReply(getTreeReply);
    m_leftInfo.setInfoLastReply(getTreeReply);

    m_rootNode = getTreeReply.tree();
    m_leftBrowser.setCdt(m_rootNode);
    m_rightBrowser.setCdt(m_rootNode);
    m_rootNodeRecevied = time(NULL);
    m_communicating = false;    
    m_mutex.unlock(); 
}

void
S9sCommander::updateObject()
{
    S9sString path;
    bool      needToRefresh;

    if (m_rightInfo.isVisible())
    {
        path = m_leftBrowser.selectedNodeFullPath();

        needToRefresh = path != m_rightInfo.objectPath();
        if (time(NULL) - m_rightInfo.objectSetTime() > 15)
            needToRefresh = true;

        if (needToRefresh)
            updateObject(path, m_rightInfo);
    }
    
    if (m_leftInfo.isVisible())
    {
        path = m_rightBrowser.selectedNodeFullPath();

        needToRefresh = path != m_leftInfo.objectPath();
        if (time(NULL) - m_leftInfo.objectSetTime() > 15)
            needToRefresh = true;

        if (needToRefresh)
            updateObject(path, m_leftInfo);
    }
}

void
S9sCommander::updateObject(
        const S9sString &path,
        S9sInfoPanel    &target)
{
    S9sMutexLocker locker(m_networkMutex);    
    S9sVariantMap  theMap;
    S9sRpcReply    getObjectReply;
    
    /*
     *
     */
    m_mutex.lock();
    target.setInfoRequestName("getObject");
    m_mutex.unlock();

    /*
     *
     */
    m_client.getObject(path);
    getObjectReply = m_client.reply();
    theMap = getObjectReply.getObject();
   
    /*
     *
     */
    m_mutex.lock();
    target.setInfoRequestName("");
    target.setInfoLastReply(getObjectReply);

    if (getObjectReply.isOk())
        target.setInfoObject(path, theMap);
    else
        target.setInfoObject(path, getObjectReply);
    
    m_mutex.unlock();
}

/**
 * \returns True if the program should continue refreshing the screen, false to
 *   exit.
 */
bool 
S9sCommander::refreshScreen()
{
    //S9sMutexLocker   locker(m_mutex);    
    startScreen();
    printHeader();

    m_leftBrowser.setSize(width() / 2, height() - 2);
    m_rightBrowser.setSize(width() / 2, height() - 2);
    
    m_leftInfo.setSize(width() / 2, height() - 2);
    m_rightInfo.setSize(width() / 2, height() - 2);

    m_rightInfo.setInfoNode(m_leftBrowser.selectedNode());
    m_leftInfo.setInfoNode(m_rightBrowser.selectedNode());

    for (int idx = 0u; idx < height() - 2; ++idx)
    {
        if (m_leftBrowser.isVisible())
            m_leftBrowser.printLine(idx);
        else if (m_leftInfo.isVisible())
            m_leftInfo.printLine(idx);

        if (m_rightBrowser.isVisible())
            m_rightBrowser.printLine(idx);
        else if (m_rightInfo.isVisible())
            m_rightInfo.printLine(idx);

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
            if (m_leftBrowser.hasFocus())
            {
                m_leftBrowser.setHasFocus(false);
                m_rightBrowser.setHasFocus(true);
            } else {
                m_leftBrowser.setHasFocus(true);
                m_rightBrowser.setHasFocus(false);
            }
            return;

        case 'r':
        case 'R':
            if (m_rightInfo.isVisible())
            {
                m_rightBrowser.setVisible(true);
                m_rightInfo.setVisible(false);
            } else {
                m_rightBrowser.setVisible(false);
                m_rightInfo.setVisible(true);
            }
            break;
        
        case 'l':
        case 'L':
            if (m_leftInfo.isVisible())
            {
                m_leftBrowser.setVisible(true);
                m_leftInfo.setVisible(false);
            } else {
                m_leftBrowser.setVisible(false);
                m_leftInfo.setVisible(true);
            }
            break;

        case 'j':
        case 'J':
            m_leftInfo.setShowJson(!m_leftInfo.showJson());
            m_rightInfo.setShowJson(!m_rightInfo.showJson());
            break;

        case 'd':
        case 'D':
            // Turning on and off the debug mode.
            m_viewDebug = !m_viewDebug;
            break;
    }

    if (m_leftBrowser.hasFocus())
        m_leftBrowser.processKey(key);
    
    if (m_rightBrowser.hasFocus())
        m_rightBrowser.processKey(key);
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
