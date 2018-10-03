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
#include "s9scalc.h"

#include "S9sDateTime"
#include <unistd.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sCalc::S9sCalc(
        S9sRpcClient            &client) : 
    S9sDisplay(true),
    m_client(client)
{
}

S9sCalc::~S9sCalc()
{
}

void
S9sCalc::setSpreadsheetName(
        const S9sString &name)
{
    m_spreadsheetName = name;
}

S9sString
S9sCalc::spreadsheetName() const
{
    return m_spreadsheetName;
}

void
S9sCalc::main()
{
    S9sRpcReply reply;
    bool        success;

    success = m_client.getSpreadsheet();
    if (!success)
    {
        m_errorString = "Error getting spreadsheet.";
    } else {
        reply = m_client.reply();
        m_spreadsheet = reply["spreadsheet"].toVariantMap();
        //m_errorString = reply["spreadsheet"].toVariantMap().toString();
        //m_errorString.replace("\n", "\r\n");
        //::printf("-> \n%s\n", STR(m_errorString));
        //sleep(100);
    }


    start();

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
            sleep(100);
            //usleep(10000);

        }
}

/**
 * Virtual function that called when the user pressed a key on the keyboard.
 */
void
S9sCalc::processKey(
        int key)
{
    switch (key)
    {
        case 'q':
        case 'Q':
            exit(0);
            break;
    }
}

void 
S9sCalc::processButton(
        uint button, 
        uint x, 
        uint y)
{
    S9sDisplay::processButton(button, x, y);
}

/**
 * \returns True if the program should continue refreshing the screen, false to
 *   exit.
 *
 * Virtual function that should paint the whole screen once. The mutex is 
 * locked when this method is called.
 */
bool
S9sCalc::refreshScreen()
{
    startScreen();
    printHeader();

    m_spreadsheet.setScreenSize(columns(), rows() - 3);
    m_spreadsheet.print();
    //printMiddle();

    printFooter();

    return true;
}

/**
 * Printing the top part of the screen.
 */
void
S9sCalc::printHeader()
{
    S9sDateTime dt = S9sDateTime::currentDateTime();
    S9sString   title = "S9S Calc";
    const char *bold = TERM_SCREEN_TITLE_BOLD;
    const char *normal = TERM_SCREEN_TITLE;

    if (!spreadsheetName().empty())
        title = spreadsheetName();

    ::printf("%s%s%s ", bold, STR(title), normal);
    ::printf("%s ", STR(dt.toString(S9sDateTime::LongTimeFormat)));
        ::printf("0x%08x ",      lastKeyCode());
        ::printf("%02dx%02d ",   columns(), rows());

    printNewLine();
    
    // The editor comes here...
    //printNewLine();
}

void
S9sCalc::printFooter()
{
    ::printf("%s", STR(m_errorString));
}
